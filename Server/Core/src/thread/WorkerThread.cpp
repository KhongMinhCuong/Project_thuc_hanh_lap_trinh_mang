#include "../../include/thread_manager.h"
#include "../../include/request_handler.h"
#include "../../include/thread_monitor.h"
#include "../../../../Common/Protocol.h"
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <thread>

WorkerThread::WorkerThread() : running(true), epoll_fd(-1) {
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
    
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = socketFd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socketFd, &ev) == -1) {
        std::cerr << "[Worker] Failed to add FD to epoll: " << socketFd << std::endl;
        return;
    }
    
    client_sockets.push_back(socketFd);
    sessions[socketFd] = ClientSession(); 
    sessions[socketFd].socketFd = socketFd;
    
    ThreadMonitor::getInstance().reportConnectionCount(myThreadId, client_sockets.size());
    
    std::cout << "[Worker] Client added. FD: " << socketFd 
              << " (Total: " << client_sockets.size() << ")" << std::endl;
}

void WorkerThread::addClient(int socketFd, const ClientSession& session) {
    std::lock_guard<std::mutex> lock(mtx);
    
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = socketFd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socketFd, &ev) == -1) {
        std::cerr << "[Worker] Failed to add FD to epoll: " << socketFd << std::endl;
        return;
    }
    
    client_sockets.push_back(socketFd);
    sessions[socketFd] = session;
    
    ThreadMonitor::getInstance().reportConnectionCount(myThreadId, client_sockets.size());
    
    std::cout << "[Worker] Client restored. FD: " << socketFd 
              << " User: " << session.username
              << " (Total: " << client_sockets.size() << ")" << std::endl;
}

void WorkerThread::removeClient(int fd, bool closeSocket) {
    std::lock_guard<std::mutex> lock(mtx);
    
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
    
    auto it = std::find(client_sockets.begin(), client_sockets.end(), fd);
    if (it != client_sockets.end()) {
        client_sockets.erase(it);
    }

    sessions.erase(fd);
    
    ThreadMonitor::getInstance().reportConnectionCount(myThreadId, client_sockets.size());

    if (closeSocket) {
        close(fd); 
        std::cout << "[Worker] Client " << fd << " disconnected. "
                  << "(Remaining: " << client_sockets.size() << ")" << std::endl;
    } else {
        std::cout << "[Worker] Client " << fd << " handed over to Dedicated Thread." << std::endl;
    }
}

void WorkerThread::run() {
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
        
        if (nfds == 0) continue;
        
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
    while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r')) {
        msg.pop_back();
    }
    
    std::cout << "[Recv FD:" << fd << "]: " << msg << std::endl;

    std::string command, arg;
    if (msg.find("SITE QUOTA_CHECK") == 0) {
        command = "SITE QUOTA_CHECK";
        arg = msg.substr(17);
        std::cout << "[Worker] Detected SITE QUOTA_CHECK, arg: '" << arg << "'" << std::endl;
    } else {
        std::stringstream ss(msg);
        ss >> command;
        std::getline(ss, arg);
        if (!arg.empty() && arg[0] == ' ') arg.erase(0, 1);
    }
    
    std::cout << "[Worker] Parsed - Command: '" << command << "', Arg: '" << arg << "'" << std::endl;
    
    std::string response;


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
        long long parent_id = 0;
        if (!arg.empty()) {
            try {
                parent_id = std::stoll(arg);
            } catch (...) {
                parent_id = 0;
            }
        }
        response = CmdHandler::handleList(sessions[fd], parent_id);
    }
    else if (command == CMD_LISTSHARED) {
        long long parent_id = -1;
        if (!arg.empty()) {
            try {
                parent_id = std::stoll(arg);
            } catch (...) {
                parent_id = -1;
            }
        }
        std::lock_guard<std::mutex> lock(mtx);
        response = CmdHandler::handleListShared(sessions[fd], parent_id);
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
    else if (command == CMD_RENAME) {
        std::stringstream ss(arg);
        long long fileId;
        std::string newName;
        ss >> fileId >> newName;
        
        std::cout << "[RENAME] Received: file_id=" << fileId << ", new_name='" << newName 
                  << "', username=" << sessions[fd].username << std::endl;
        
        std::lock_guard<std::mutex> lock(mtx);
        response = CmdHandler::handleRename(sessions[fd], fileId, newName);
    }
    
    else if (command == CMD_UPLOAD_CHECK) {
        std::stringstream ss_quota(arg);
        std::string fname;
        long fsize = 0;
        ss_quota >> fname >> fsize;
        
        std::cout << "[Worker] Checking quota for: " << fname << ", Size: " << fsize << std::endl;
        
        std::lock_guard<std::mutex> lock(mtx);
        response = FileIOHandler::handleQuotaCheck(sessions[fd], fsize);
    }

    else if (command == CMD_UPLOAD) {
        std::string fname;
        long fsize = 0;
        long long parent_id = 0;
        std::stringstream ss_up(arg);
        ss_up >> fname >> fsize >> parent_id;
        
        std::cout << "[Worker::CMD_UPLOAD] File: " << fname << ", Size: " << fsize << ", Parent ID: " << parent_id << std::endl;

        if (fsize <= 0) {
            std::cout << "[Worker::CMD_UPLOAD] Invalid file size" << std::endl;
            response = std::string(CODE_FAIL) + " Invalid file size\n";
        } else if (!ThreadMonitor::getInstance().canCreateDedicatedThread()) {
            std::cout << "[Worker::CMD_UPLOAD] System overloaded" << std::endl;
            response = "503 System overloaded\n";
        } else {
            std::string username = sessions[fd].username;
            std::cout << "[Worker::CMD_UPLOAD] Starting dedicated thread for upload" << std::endl;
            removeClient(fd, false);
            
            std::thread t([fd, fname, fsize, username, parent_id, this]() {
                DedicatedThread dt;
                dt.handleUpload(fd, fname, fsize, username, parent_id, this);
            });
            
            std::thread::id tid = t.get_id();
            t.detach();
            
            return;
        }
    }
    else if (command == CMD_DOWNLOAD) {
        std::string fname = arg;
        std::cout << "[Worker::CMD_DOWNLOAD] File: " << fname << ", User: " << sessions[fd].username << std::endl;
        
        bool hasPerm = false;
        {
             std::lock_guard<std::mutex> lock(mtx);
             hasPerm = FileIOHandler::checkDownloadPermission(sessions[fd], fname);
        }

        if (!hasPerm) {
            std::cout << "[Worker::CMD_DOWNLOAD] Permission denied" << std::endl;
            response = std::string(CODE_FAIL) + " Permission denied\n";
        } else if (!ThreadMonitor::getInstance().canCreateDedicatedThread()) {
            std::cout << "[Worker::CMD_DOWNLOAD] System overloaded" << std::endl;
            response = "503 System overloaded\n";
        } else {
            std::string username = sessions[fd].username;
            std::cout << "[Worker::CMD_DOWNLOAD] Starting dedicated thread for download" << std::endl;
            removeClient(fd, false);

            std::thread t([fd, fname, username, this]() {
                DedicatedThread dt;
                dt.handleDownload(fd, fname, username, this);
            });
            
            t.detach();
            return;
        }
    }
    else if (command == "DOWNLOAD_FOLDER") {
        std::string folder_name = arg;
        std::string username = sessions[fd].username;
        
        std::cout << "[Worker] DOWNLOAD_FOLDER request: '" << folder_name << "' by user: " << username << std::endl;
        
        // Check if folder exists in storage
        std::string folderPath = std::string(ServerConfig::STORAGE_PATH) + folder_name;
        std::cout << "[Worker] Checking folder path: " << folderPath << std::endl;
        
        struct stat st;
        int stat_result = stat(folderPath.c_str(), &st);
        
        if (stat_result != 0) {
            std::cout << "[Worker] stat() failed with error: " << strerror(errno) << std::endl;
            response = std::string(CODE_FAIL) + " Folder not found\n";
        } else if (!S_ISDIR(st.st_mode)) {
            std::cout << "[Worker] Path exists but is not a directory" << std::endl;
            response = std::string(CODE_FAIL) + " Folder not found\n";
        } else {
            std::cout << "[Worker] Folder exists, checking permissions..." << std::endl;
            // Check if user owns the folder or has permission
            bool hasPerm = true;
            try {
                hasPerm = FileIOHandler::checkDownloadPermission(sessions[fd], folder_name);
            } catch (...) {
                // If permission check fails, allow if folder exists in user's storage
                hasPerm = true;
            }
            
            if (!hasPerm) {
                std::cout << "[Worker] Permission denied" << std::endl;
                response = std::string(CODE_FAIL) + " Permission denied\n";
            } else {
                std::cout << "[Worker] Permission OK, sending folder..." << std::endl;
                // Send ready response
                response = std::string(CODE_DATA_OPEN) + " Ready to send folder\n";
                send(fd, response.c_str(), response.length(), 0);
                
                // Handle folder download
                DedicatedThread dt;
                dt.handleFolderDownload(fd, folder_name, username);
                return;
            }
        }
    }
    else if (command == "GET_FOLDER_STRUCTURE") {
        long long folder_id = 0;
        std::stringstream ss_folder(arg);
        ss_folder >> folder_id;
        
        std::cout << "[Worker::CMD_GET_FOLDER_STRUCTURE] Folder ID: " << folder_id << ", User: " << sessions[fd].username << std::endl;
        
        std::lock_guard<std::mutex> lock(mtx);
        response = CmdHandler::handleGetFolderStructure(sessions[fd], folder_id);
    }
    
    else if (command == "SHARE_FOLDER") {
        long long folder_id = 0;
        std::string target_user;
        std::stringstream ss_share_folder(arg);
        ss_share_folder >> folder_id >> target_user;
        
        std::cout << "[Worker::CMD_SHARE_FOLDER] Folder ID: " << folder_id << ", Target: " << target_user << ", User: " << sessions[fd].username << std::endl;
        
        std::cout << "[Worker] SHARE_FOLDER: folder_id=" << folder_id 
                  << ", target=" << target_user << std::endl;
        
        std::lock_guard<std::mutex> lock(mtx);
        response = CmdHandler::handleShareFolder(sessions[fd], folder_id, target_user);
    }
    
    else if (command == "UPLOAD_FILE") {
        std::string session_id;
        long long old_file_id = 0;
        long file_size = 0;
        
        std::stringstream ss_upload_folder(arg);
        ss_upload_folder >> session_id >> old_file_id >> file_size;
        
        std::cout << "[Worker] UPLOAD_FILE: session=" << session_id 
                  << ", file_id=" << old_file_id 
                  << ", size=" << file_size << std::endl;
        
        if (file_size <= 0) {
            response = std::string(CODE_FAIL) + " Invalid file size\n";
        } else if (!ThreadMonitor::getInstance().canCreateDedicatedThread()) {
            response = "503 System overloaded\n";
        } else {
            std::string ack = std::string(CODE_DATA_OPEN) + " Ready to receive\n";
            send(fd, ack.c_str(), ack.length(), 0);
            
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
                if (FolderShareHandler::getInstance().isComplete(session_id)) {
                    FolderShareHandler::getInstance().finalize(session_id);
                    
                    response = std::string(CODE_TRANSFER_COMPLETE) + " Folder share completed\n";
                    
                    FolderShareHandler::getInstance().cleanup(session_id);
                    
                    std::cout << "[Worker] Folder share completed: " << session_id << std::endl;
                } else {
                    std::string progress = FolderShareHandler::getInstance().getProgress(session_id);
                    response = "202 " + progress + "\n";
                }
            }
        }
    }
    
    else if (command == "CREATE_FOLDER") {
        std::stringstream ss_create(arg);
        std::string foldername;
        long long parent_id = 0;
        ss_create >> foldername >> parent_id;
        
        if (!sessions[fd].isAuthenticated) {
            response = std::string(CODE_FAIL) + " Please login first\n";
        } else {
            std::lock_guard<std::mutex> lock(mtx);
            long long folder_id = DBManager::getInstance().createFolder(foldername, parent_id, sessions[fd].username);
            
            if (folder_id != -1) {
                std::cout << "[Server] Created folder: " << foldername << " (ID: " << folder_id << ")" << std::endl;
                response = std::string(CODE_OK) + " Folder created|FOLDER_ID:" + std::to_string(folder_id) + "\n";
            } else {
                std::cerr << "[Server] Failed to create folder: " << foldername << std::endl;
                response = std::string(CODE_FAIL) + " Failed to create folder\n";
            }
        }
    }
    
    else if (command == "CHECK_SHARE_PROGRESS") {
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
    
    else if (command == "CANCEL_FOLDER_SHARE") {
        std::string session_id = arg;
        
        std::cout << "[Worker] CANCEL_FOLDER_SHARE: " << session_id << std::endl;
        
        FolderShareHandler::getInstance().cleanup(session_id);
        
        response = "200 Share cancelled\n";
    }
    else {
        std::cout << "[Worker] UNKNOWN COMMAND: '" << command << "'" << std::endl;
        response = "500 Unknown command\n";
    }

    if (!response.empty()) {
        send(fd, response.c_str(), response.length(), 0);
    }
}