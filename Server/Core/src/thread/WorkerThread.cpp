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
#include <thread>

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
    // Đăng ký thread ID thực tế
    myThreadId = std::this_thread::get_id();
    ThreadMonitor::getInstance().registerWorkerThread(this, myThreadId);
    
    std::cout << "[Worker] Started event loop with epoll." << std::endl;
    
    const int MAX_EVENTS = 50;
    struct epoll_event events[MAX_EVENTS];
    
    while (running) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, 1000);
        
        if (nfds == -1) {
            if (errno == EINTR) continue;
            std::cerr << "[Worker] epoll_wait error: " << strerror(errno) << std::endl;
            continue;
        }
        
        if (nfds == 0) continue; // Timeout
        
        for (int i = 0; i < nfds; i++) {
            int fd = events[i].data.fd;
            
            if (events[i].events & EPOLLIN) {
                handleClientMessage(fd);
            }
            
            if (events[i].events & (EPOLLERR | EPOLLHUP)) {
                std::cout << "[Worker] Socket error on FD: " << fd << std::endl;
                removeClient(fd, true);
            }
        }
    }
    
    if (epoll_fd != -1) close(epoll_fd);
}

void WorkerThread::handleClientMessage(int fd) {
    char buffer[1025] = {0};
    int valread = read(fd, buffer, 1024);

    if (valread <= 0) {
        removeClient(fd, true);
        return;
    }

    std::string msg(buffer);
    // Xóa ký tự xuống dòng
    while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r')) {
        msg.pop_back();
    }
    
    std::cout << "[Recv FD:" << fd << "]: " << msg << std::endl;

    // Parse lệnh
    std::string command, arg;
    if (msg.substr(0, 17) == "SITE QUOTA_CHECK ") {
        command = "SITE QUOTA_CHECK";
        arg = msg.substr(17);
    } else {
        std::stringstream ss(msg);
        ss >> command;
        std::getline(ss, arg);
        if (!arg.empty() && arg[0] == ' ') arg.erase(0, 1);
    }
    
    std::string response;

    // --- XỬ LÝ LỆNH ---

    if (command == CMD_USER) {
        std::lock_guard<std::mutex> lock(mtx);
        response = AuthHandler::handleUser(fd, sessions[fd], arg);
    } 
    else if (command == CMD_PASS) {
        std::lock_guard<std::mutex> lock(mtx);
        response = AuthHandler::handlePass(fd, sessions[fd], arg);
    }
    else if (command == CMD_REGISTER) {
        std::stringstream ss_reg(arg);
        std::string u, p;
        ss_reg >> u >> p;
        if (!u.empty() && !p.empty()) 
             response = AuthHandler::handleRegister(u, p);
        else response = std::string(CODE_FAIL) + " Invalid format\n";
    }
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
        std::stringstream ss_share(arg);
        std::string fname, target;
        ss_share >> fname >> target;
        std::lock_guard<std::mutex> lock(mtx);
        response = CmdHandler::handleShare(sessions[fd], fname, target);
    }
    else if (command == CMD_DELETE) {
        std::lock_guard<std::mutex> lock(mtx);
        response = CmdHandler::handleDelete(sessions[fd], arg);
    }
    
    // --- ĐOẠN FIX LỖI TIMEOUT ---
    else if (command == CMD_UPLOAD_CHECK) {
        // Parse: filename filesize
        std::stringstream ss_quota(arg);
        std::string fname;
        long fsize = 0;
        ss_quota >> fname >> fsize;
        
        std::cout << "[Worker] Checking quota for: " << fname << ", Size: " << fsize << std::endl;
        
        std::lock_guard<std::mutex> lock(mtx);
        // Gọi hàm xử lý và nhận phản hồi ngay lập tức
        response = FileIOHandler::handleQuotaCheck(sessions[fd], fsize);
    }
    // -----------------------------

    else if (command == CMD_UPLOAD) { // STOR
        std::string fname;
        long fsize = 0;
        std::stringstream ss_up(arg);
        ss_up >> fname >> fsize;

        if (fsize <= 0) {
            response = std::string(CODE_FAIL) + " Invalid file size\n";
        } else if (!ThreadMonitor::getInstance().canCreateDedicatedThread()) {
            response = "503 System overloaded\n";
        } else {
            // Chuyển sang Dedicated Thread
            std::string username = sessions[fd].username;
            removeClient(fd, false); // false = không đóng socket
            
            std::thread t([fd, fname, fsize, username, this]() {
                DedicatedThread dt;
                dt.handleUpload(fd, fname, fsize, username, this);
            });
            
            std::thread::id tid = t.get_id();
            t.detach(); // Detach để thread chạy ngầm
            // ThreadMonitor::getInstance().registerDedicatedThread(tid, std::move(t)); // Nếu dùng joinable thread
            
            return; // Không gửi response ở đây, DedicatedThread sẽ lo
        }
    }
    else if (command == CMD_DOWNLOAD) { // RETR
        std::string fname = arg;
        bool hasPerm = false;
        {
             std::lock_guard<std::mutex> lock(mtx);
             hasPerm = FileIOHandler::checkDownloadPermission(sessions[fd], fname);
        }

        if (!hasPerm) {
            response = std::string(CODE_FAIL) + " Permission denied\n";
        } else if (!ThreadMonitor::getInstance().canCreateDedicatedThread()) {
            response = "503 System overloaded\n";
        } else {
            std::string username = sessions[fd].username;
            removeClient(fd, false);

            std::thread t([fd, fname, username, this]() {
                DedicatedThread dt;
                dt.handleDownload(fd, fname, username, this);
            });
            
            t.detach();
            return;
        }
    }
    else if (command == "GET_FOLDER_STRUCTURE") {
        // Format: GET_FOLDER_STRUCTURE <folder_id>
        long long folder_id = 0;
        std::stringstream ss_folder(arg);
        ss_folder >> folder_id;
        
        std::cout << "[Worker] GET_FOLDER_STRUCTURE: folder_id=" << folder_id << std::endl;
        
        std::lock_guard<std::mutex> lock(mtx);
        response = CmdHandler::handleGetFolderStructure(sessions[fd], folder_id);
    }
    
    // ===== FOLDER SHARE COMMAND 2: SHARE_FOLDER =====
    else if (command == "SHARE_FOLDER") {
        // Format: SHARE_FOLDER <folder_id> <target_username>
        long long folder_id = 0;
        std::string target_user;
        std::stringstream ss_share_folder(arg);
        ss_share_folder >> folder_id >> target_user;
        
        std::cout << "[Worker] SHARE_FOLDER: folder_id=" << folder_id 
                  << ", target=" << target_user << std::endl;
        
        std::lock_guard<std::mutex> lock(mtx);
        response = CmdHandler::handleShareFolder(sessions[fd], folder_id, target_user);
    }
    
    // ===== FOLDER SHARE COMMAND 3: UPLOAD_FOLDER_FILE =====
    else if (command == "UPLOAD_FOLDER_FILE") {
        // Format: UPLOAD_FOLDER_FILE <session_id> <old_file_id> <file_size>
        std::string session_id;
        long long old_file_id = 0;
        long file_size = 0;
        
        std::stringstream ss_upload_folder(arg);
        ss_upload_folder >> session_id >> old_file_id >> file_size;
        
        std::cout << "[Worker] UPLOAD_FOLDER_FILE: session=" << session_id 
                  << ", file_id=" << old_file_id 
                  << ", size=" << file_size << std::endl;
        
        if (file_size <= 0) {
            response = std::string(CODE_FAIL) + " Invalid file size\n";
        } else if (!ThreadMonitor::getInstance().canCreateDedicatedThread()) {
            response = "503 System overloaded\n";
        } else {
            // Gửi ACK để client biết server sẵn sàng nhận
            std::string ack = std::string(CODE_DATA_OPEN) + " Ready to receive\n";
            send(fd, ack.c_str(), ack.length(), 0);
            
            // Nhận binary data
            char* file_buffer = new char[file_size];
            long total_received = 0;
            
            while (total_received < file_size) {
                int bytes_read = read(fd, file_buffer + total_received, 
                                     file_size - total_received);
                if (bytes_read <= 0) {
                    std::cerr << "[Worker] Connection lost during folder file transfer" << std::endl;
                    delete[] file_buffer;
                    removeClient(fd, true);
                    return;
                }
                total_received += bytes_read;
            }
            
            std::cout << "[Worker] Received " << total_received << " bytes for folder file" << std::endl;
            
            // Lưu file qua FolderShareHandler
            bool success = FolderShareHandler::getInstance().receiveFile(
                session_id,
                old_file_id,
                file_buffer,
                file_size
            );
            
            delete[] file_buffer;
            
            if (!success) {
                response = std::string(CODE_FAIL) + " Failed to save folder file\n";
            } else {
                // Kiểm tra xem đã hoàn thành chưa
                if (FolderShareHandler::getInstance().isComplete(session_id)) {
                    // Hoàn thành - finalize share
                    FolderShareHandler::getInstance().finalize(session_id);
                    
                    response = std::string(CODE_TRANSFER_COMPLETE) + " Folder share completed\n";
                    
                    // Cleanup session
                    FolderShareHandler::getInstance().cleanup(session_id);
                    
                    std::cout << "[Worker] Folder share completed: " << session_id << std::endl;
                } else {
                    // Chưa hoàn thành - gửi progress
                    std::string progress = FolderShareHandler::getInstance().getProgress(session_id);
                    response = "202 " + progress + "\n";
                }
            }
        }
    }
    
    // ===== FOLDER SHARE COMMAND 4: CHECK_SHARE_PROGRESS (Optional) =====
    else if (command == "CHECK_SHARE_PROGRESS") {
        // Format: CHECK_SHARE_PROGRESS <session_id>
        std::string session_id = arg;
        
        std::cout << "[Worker] CHECK_SHARE_PROGRESS: " << session_id << std::endl;
        
        auto session_ptr = FolderShareHandler::getInstance().getSession(session_id);
        
        if (!session_ptr) {
            response = std::string(CODE_FAIL) + " Session not found\n";
        } else {
            std::string progress = FolderShareHandler::getInstance().getProgress(session_id);
            response = "200 " + progress + "\n";
        }
    }
    
    // ===== FOLDER SHARE COMMAND 5: CANCEL_FOLDER_SHARE (Optional) =====
    else if (command == "CANCEL_FOLDER_SHARE") {
        // Format: CANCEL_FOLDER_SHARE <session_id>
        std::string session_id = arg;
        
        std::cout << "[Worker] CANCEL_FOLDER_SHARE: " << session_id << std::endl;
        
        FolderShareHandler::getInstance().cleanup(session_id);
        
        response = "200 Share cancelled\n";
    }
    else if (command == "MKDIR" || command == "CREATE_FOLDER") {
        std::stringstream ss_mkdir(arg);
        std::string folder_name;
        long long parent_id = 1;
        
        ss_mkdir >> folder_name;
        
        if (ss_mkdir >> parent_id) {
        }
        
        std::cout << "[Worker] CREATE_FOLDER: name=" << folder_name 
                << ", parent_id=" << parent_id << std::endl;
        
        std::lock_guard<std::mutex> lock(mtx);
        response = CmdHandler::handleCreateFolder(sessions[fd], folder_name, parent_id);
    }
    else {
        response = "500 Unknown command\n";
    }

    if (!response.empty()) {
        send(fd, response.c_str(), response.length(), 0);
    }
}