#include "../../include/thread_manager.h"
#include "../../include/request_handler.h"  // Gọi các Handler
#include "../../include/thread_monitor.h"   // Báo cáo cho Monitor
#include "../../../../Common/Protocol.h"    // Gọi lệnh CMD_*, CODE_*
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include <algorithm>

WorkerThread::WorkerThread() : running(true), epoll_fd(-1) {
    // Tạo epoll instance
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        std::cerr << "[Worker] Failed to create epoll instance" << std::endl;
    }
}

void WorkerThread::stop() {
    running = false;
}

void WorkerThread::addClient(int socketFd) {
    std::lock_guard<std::mutex> lock(mtx);
    
    // Thêm vào epoll
    struct epoll_event ev;
    ev.events = EPOLLIN;  // Quan tâm sự kiện đọc
    ev.data.fd = socketFd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socketFd, &ev) == -1) {
        std::cerr << "[Worker] Failed to add FD to epoll: " << socketFd << std::endl;
        return;
    }
    
    client_sockets.push_back(socketFd);
    sessions[socketFd] = ClientSession(); 
    sessions[socketFd].socketFd = socketFd;
    
    // Báo cáo cho Monitor
    ThreadMonitor::getInstance().reportConnectionCount(myThreadId, client_sockets.size());
    
    std::cout << "[Worker] Client added. FD: " << socketFd 
              << " (Total: " << client_sockets.size() << ")" << std::endl;
}

// Overload: addClient với session khôi phục
void WorkerThread::addClient(int socketFd, const ClientSession& session) {
    std::lock_guard<std::mutex> lock(mtx);
    
    // Thêm vào epoll
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = socketFd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socketFd, &ev) == -1) {
        std::cerr << "[Worker] Failed to add FD to epoll: " << socketFd << std::endl;
        return;
    }
    
    client_sockets.push_back(socketFd);
    sessions[socketFd] = session;  // Khôi phục session cũ
    
    // Báo cáo cho Monitor
    ThreadMonitor::getInstance().reportConnectionCount(myThreadId, client_sockets.size());
    
    std::cout << "[Worker] Client restored. FD: " << socketFd 
              << " User: " << session.username
              << " Auth: " << session.isAuthenticated
              << " (Total: " << client_sockets.size() << ")" << std::endl;
}

void WorkerThread::removeClient(int fd, bool closeSocket) {
    std::lock_guard<std::mutex> lock(mtx);
    
    // 1. Xóa khỏi epoll
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
    
    // 2. Xóa khỏi danh sách theo dõi
    auto it = std::find(client_sockets.begin(), client_sockets.end(), fd);
    if (it != client_sockets.end()) {
        client_sockets.erase(it);
    }

    // 3. Xóa session
    sessions.erase(fd);
    
    // 4. Báo cáo cho Monitor
    ThreadMonitor::getInstance().reportConnectionCount(myThreadId, client_sockets.size());

    // 5. Xử lý socket
    if (closeSocket) {
        close(fd); 
        std::cout << "[Worker] Client " << fd << " disconnected. "
                  << "(Remaining: " << client_sockets.size() << ")" << std::endl;
    } else {
        std::cout << "[Worker] Client " << fd << " handed over to Dedicated Thread." << std::endl;
    }
}

void WorkerThread::run() {
    // Đăng ký thread ID thực tế khi worker bắt đầu chạy
    myThreadId = std::this_thread::get_id();
    ThreadMonitor::getInstance().registerWorkerThread(this, myThreadId);
    
    std::cout << "[Worker] Started event loop with epoll (I/O Multiplexing)." << std::endl;
    
    const int MAX_EVENTS = 50;  // Xử lý tối đa 50 events mỗi lần
    struct epoll_event events[MAX_EVENTS];
    
    while (running) {
        // --- BƯỚC 1: Chờ sự kiện với epoll (Timeout 1000ms) ---
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, 1000);
        
        if (nfds == -1) {
            if (errno == EINTR) continue;  // Interrupted by signal
            std::cerr << "[Worker] epoll_wait error: " << strerror(errno) << std::endl;
            continue;
        }
        
        if (nfds == 0) {
            // Timeout - không có sự kiện nào
            continue;
        }
        
        // --- BƯỚC 2: Xử lý các sự kiện ---
        for (int i = 0; i < nfds; i++) {
            int fd = events[i].data.fd;
            
            // Kiểm tra sự kiện đọc
            if (events[i].events & EPOLLIN) {
                handleClientMessage(fd);
            }
            
            // Kiểm tra lỗi hoặc ngắt kết nối
            if (events[i].events & (EPOLLERR | EPOLLHUP)) {
                std::cout << "[Worker] Socket error/hangup on FD: " << fd << std::endl;
                removeClient(fd, true);
            }
        }
    }
    
    // Đóng epoll khi kết thúc
    if (epoll_fd != -1) {
        close(epoll_fd);
    }
}

void WorkerThread::handleClientMessage(int fd) {
    char buffer[1025] = {0};
    int valread = read(fd, buffer, 1024);

    // 1. Kiểm tra ngắt kết nối
    if (valread <= 0) {
        removeClient(fd, true); // Đóng socket
        return;
    }

    // 2. Xử lý chuỗi lệnh
    std::string msg(buffer);
    // Xóa ký tự xuống dòng (\r\n)
    while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r')) {
        msg.pop_back();
    }
    
    // Log lệnh nhận được (Debug)
    std::cout << "[Recv FD:" << fd << "]: " << msg << std::endl;

    // Parse lệnh: Xử lý "SITE QUOTA_CHECK" đặc biệt
    std::string command, arg;
    
    if (msg.substr(0, 17) == "SITE QUOTA_CHECK ") {
        command = "SITE QUOTA_CHECK";
        arg = msg.substr(17); // Lấy phần sau "SITE QUOTA_CHECK "
    } else {
        // Parse thông thường: COMMAND arg1 arg2...
        std::stringstream ss(msg);
        ss >> command;
        std::getline(ss, arg);
        if (!arg.empty() && arg[0] == ' ') arg.erase(0, 1); // Trim space đầu
    }
    
    std::cout << "[Parse] Command: '" << command << "', Arg: '" << arg << "'" << std::endl;

    std::string response;

    // --- ĐIỀU HƯỚNG LỆNH (DISPATCHER) ---

    // A. Nhóm AUTH (AuthHandler)
    if (command == CMD_USER) {
        std::lock_guard<std::mutex> lock(mtx);
        response = AuthHandler::handleUser(fd, sessions[fd], arg);
    } 
    else if (command == CMD_PASS) {
        std::lock_guard<std::mutex> lock(mtx);
        response = AuthHandler::handlePass(fd, sessions[fd], arg);
    }
    else if (command == CMD_REGISTER) {
        // Cần tách arg thành user và pass (Ví dụ: "admin 123456")
        std::stringstream ss_reg(arg);
        std::string u, p;
        ss_reg >> u >> p;
        if (!u.empty() && !p.empty()) 
             response = AuthHandler::handleRegister(u, p);
        else response = std::string(CODE_FAIL) + " Invalid format\n";
    }

    // B. Nhóm COMMAND (CmdHandler)
    else if (command == CMD_LIST) {
        std::lock_guard<std::mutex> lock(mtx);
        response = CmdHandler::handleList(sessions[fd]);
    }
    else if (command == CMD_LISTSHARED) {
        std::lock_guard<std::mutex> lock(mtx);
        response = CmdHandler::handleListShared(sessions[fd]);
    }
    else if (command == CMD_SEARCH) {
        std::lock_guard<std::mutex> lock(mtx);
        response = CmdHandler::handleSearch(sessions[fd], arg);
    }
    else if (command == CMD_SHARE) {
        // Tách arg: filename targetUser
        std::stringstream ss_share(arg);
        std::string fname, target;
        ss_share >> fname >> target;
        std::lock_guard<std::mutex> lock(mtx);
        response = CmdHandler::handleShare(sessions[fd], fname, target);
    }
    else if (command == CMD_DELETE) {
        // Xóa file: DELETE filename
        std::lock_guard<std::mutex> lock(mtx);
        response = CmdHandler::handleDelete(sessions[fd], arg);
    }

    // C. Nhóm IO CHECK (FileIOHandler)
    else if (command == CMD_UPLOAD_CHECK) {
        std::cout << "[Worker] Processing UPLOAD_CHECK, arg: " << arg << std::endl;
        // Client gửi: SITE QUOTA_CHECK filename filesize
        std::stringstream ss_quota(arg);
        std::string fname;
        long fsize = 0;
        ss_quota >> fname >> fsize;
        
        std::cout << "[Worker] Parsed - File: " << fname << ", Size: " << fsize << std::endl;
        
        std::lock_guard<std::mutex> lock(mtx);
        response = FileIOHandler::handleQuotaCheck(sessions[fd], fsize);
    }

    // D. Nhóm HEAVY IO (Chuyển Thread)
    else if (command == CMD_UPLOAD) {
        // Lệnh: STOR filename filesize
        std::string fname;
        long fsize = 0;
        std::stringstream ss_up(arg);
        ss_up >> fname >> fsize;

        if (fsize <= 0) {
            response = std::string(CODE_FAIL) + " Invalid file size\n";
        } else {
            // Kiểm tra hệ thống có quá tải không
            if (!ThreadMonitor::getInstance().canCreateDedicatedThread()) {
                response = "503 Service temporarily unavailable (too many uploads)\n";
                std::cout << "[Worker] Rejected upload request: system overloaded" << std::endl;
            } else {
                // -- QUY TRÌNH CHUYỂN GIAO --
                // 1. Ngừng theo dõi ở Worker (KHÔNG đóng socket)
                std::string username = sessions[fd].username;
                removeClient(fd, false);
                
                // 2. Tạo Dedicated Thread và đăng ký với Monitor
                std::thread t([fd, fname, fsize, username, this]() {
                    DedicatedThread dt;
                    dt.handleUpload(fd, fname, fsize, username, this);
                });
                
                // Đăng ký với Monitor để quản lý
                std::thread::id tid = t.get_id();
                ThreadMonitor::getInstance().registerDedicatedThread(tid, std::move(t));
                
                return; // Kết thúc hàm, không gửi response ở đây
            }
        }
    }
    else if (command == CMD_DOWNLOAD) {
        // Lệnh: RETR filename
        std::string fname = arg;

        // Kiểm tra quyền trước
        bool hasPerm = false;
        {
             std::lock_guard<std::mutex> lock(mtx);
             hasPerm = FileIOHandler::checkDownloadPermission(sessions[fd], fname);
        }

        if (!hasPerm) {
            response = std::string(CODE_FAIL) + " Permission denied\n";
        } else {
            // Kiểm tra hệ thống có quá tải không
            if (!ThreadMonitor::getInstance().canCreateDedicatedThread()) {
                response = "503 Service temporarily unavailable (too many downloads)\n";
                std::cout << "[Worker] Rejected download request: system overloaded" << std::endl;
            } else {
                // -- QUY TRÌNH CHUYỂN GIAO --
                std::string username = sessions[fd].username;
                removeClient(fd, false);

                std::thread t([fd, fname, username, this]() {
                    DedicatedThread dt;
                    dt.handleDownload(fd, fname, username, this);
                });
                
                // Đăng ký với Monitor để quản lý
                std::thread::id tid = t.get_id();
                ThreadMonitor::getInstance().registerDedicatedThread(tid, std::move(t));
                
                return;
            }
        }
    }
    else {
        response = "500 Unknown command\n";
    }

    // Gửi phản hồi (nếu không phải trường hợp chuyển thread)
    send(fd, response.c_str(), response.length(), 0);
}