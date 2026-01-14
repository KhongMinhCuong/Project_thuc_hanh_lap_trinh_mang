#include "../../include/thread_manager.h"
#include "../../include/db_manager.h"
#include "../../include/thread_monitor.h"
#include "../../include/server_config.h"
#include "../../../../Common/Protocol.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <unistd.h>
#include <sys/socket.h>
#include <dirent.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <endian.h>
#include <cstring>
#include <fcntl.h>
#include <cerrno>
#include <netinet/tcp.h>

#define BUFFER_SIZE ServerConfig::BUFFER_SIZE
#define STORAGE_PATH ServerConfig::STORAGE_PATH

void DedicatedThread::handleUpload(int socketFd, std::string filename, long filesize, std::string username, long long parent_id, WorkerThread* workerRef) {
    std::cout << "[SERVER] ===== UPLOAD FILE HANDLER =====" << std::endl;
    std::cout << "[SERVER] Receiving: " << filename << " (" << filesize << " bytes) from user: " << username << std::endl;
    
    ThreadMonitor::getInstance().reportDedicatedThreadStart();

    std::string path = std::string(STORAGE_PATH) + filename;
    
    // Create parent directories if filename contains path separators
    size_t lastSlash = path.find_last_of('/');
    if (lastSlash != std::string::npos) {
        std::string dirPath = path.substr(0, lastSlash);
        
        // Create directories recursively using C++17 filesystem
        namespace fs = std::filesystem;
        try {
            fs::create_directories(dirPath);
            std::cout << "[Dedicated] Created directory: " << dirPath << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[Dedicated] Failed to create directory: " << e.what() << std::endl;
            std::string err = std::string(CODE_FAIL) + " Cannot create directory on server\n";
            send(socketFd, err.c_str(), err.length(), 0);
            close(socketFd);
            ThreadMonitor::getInstance().reportDedicatedThreadEnd();
            return;
        }
    }
    
    std::ofstream outfile(path, std::ios::binary);
    
    if (!outfile.is_open()) {
        std::string err = std::string(CODE_FAIL) + " Cannot create file on server\n";
        send(socketFd, err.c_str(), err.length(), 0);
        close(socketFd);
        ThreadMonitor::getInstance().reportDedicatedThreadEnd();
        return;
    }
    
    std::string msg = std::string(CODE_DATA_OPEN) + " Ready to receive data\n";
    send(socketFd, msg.c_str(), msg.length(), 0);

    char buffer[BUFFER_SIZE];
    long totalReceived = 0;
    const long ACK_GROUP_SIZE = 1048576;
    long bytesSinceLastAck = 0;
    
    while (totalReceived < filesize) {
        int bytesRead = read(socketFd, buffer, BUFFER_SIZE);
        if (bytesRead <= 0) break;

        outfile.write(buffer, bytesRead);
        totalReceived += bytesRead;
        bytesSinceLastAck += bytesRead;
        
        if (bytesSinceLastAck >= ACK_GROUP_SIZE && totalReceived < filesize) {
            std::string ack = std::string(CODE_CHUNK_ACK) + " Received " + std::to_string(totalReceived) + " bytes\n";
            send(socketFd, ack.c_str(), ack.length(), 0);
            bytesSinceLastAck = 0;
        }
    }

    outfile.close();

    bool saved = DBManager::getInstance().addFile(filename, totalReceived, username, parent_id);
    if (!saved) {
        std::cerr << "[SERVER] Upload FAILED: Database save error for " << filename << std::endl;
    } else {
        std::cout << "[SERVER] Upload SUCCESS: " << filename << " (" << totalReceived << " bytes)" << std::endl;
    }

    msg = std::string(CODE_TRANSFER_COMPLETE) + " Upload success\n";
    send(socketFd, msg.c_str(), msg.length(), 0);
    
    ThreadMonitor::getInstance().reportBytesTransferred(filesize);
    ThreadMonitor::getInstance().reportDedicatedThreadEnd();
    
    if (workerRef) {
        ClientSession restoredSession;
        restoredSession.socketFd = socketFd;
        restoredSession.username = username;
        restoredSession.isAuthenticated = true;
        
        workerRef->addClient(socketFd, restoredSession);
        std::cout << "[Dedicated] Socket " << socketFd << " returned with session (user: " << username << ")" << std::endl;
    }
}

void DedicatedThread::sendFile(int socketFd, const std::string& fullPath, const std::string& relativePath) {
    std::cout << "[DedicatedThread] Sending file: " << relativePath << std::endl;
    
    int fd = open(fullPath.c_str(), O_RDONLY);
    if (fd < 0) {
        std::cerr << "[DedicatedThread] Failed to open file: " << fullPath << std::endl;
        return;
    }

    struct stat st;
    fstat(fd, &st);
    uint64_t fileSize = st.st_size;

    // Send TYPE_FILE
    uint8_t type = TYPE_FILE;
    send(socketFd, &type, 1, 0);

    // Send name length
    uint32_t nameLen = htonl(relativePath.length());
    send(socketFd, &nameLen, 4, 0);

    // Send file size
    uint64_t netSize = htobe64(fileSize);
    send(socketFd, &netSize, 8, 0);

    // Send name
    send(socketFd, relativePath.c_str(), relativePath.length(), 0);

    // Send file data
    char buffer[65536];
    ssize_t n;
    uint64_t totalSent = 0;
    while ((n = read(fd, buffer, sizeof(buffer))) > 0) {
        send(socketFd, buffer, n, 0);
        totalSent += n;
    }

    close(fd);
    std::cout << "[DedicatedThread] File sent: " << relativePath << " (" << totalSent << " bytes)" << std::endl;
}

void DedicatedThread::sendDirectory(int socketFd, const std::string& basePath, const std::string& relativePath) {
    std::cout << "[DedicatedThread] Sending directory: " << relativePath << std::endl;
    // Send TYPE_DIR
    uint8_t type = TYPE_DIR;
    send(socketFd, &type, 1, 0);

    // Send name length
    uint32_t nameLen = htonl(relativePath.length());
    send(socketFd, &nameLen, 4, 0);

    // Send name
    send(socketFd, relativePath.c_str(), relativePath.length(), 0);

    // Build full path
    std::string fullPath = basePath + "/" + relativePath;

    DIR* dir = opendir(fullPath.c_str());
    if (!dir) {
        std::cerr << "[DedicatedThread] Failed to open directory: " << fullPath << std::endl;
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        std::string newRelPath = relativePath + "/" + entry->d_name;
        std::string newFullPath = basePath + "/" + newRelPath;

        struct stat st;
        if (stat(newFullPath.c_str(), &st) < 0) {
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            sendDirectory(socketFd, basePath, newRelPath);
        } else {
            sendFile(socketFd, newFullPath, newRelPath);
        }
    }

    closedir(dir);
}

// Gửi file từ storage, dùng filename đã lưu trong DB
void DedicatedThread::sendFileFromDb(int socketFd, const std::string& filename, const std::string& relativePath) {
    std::string fullPath = std::string(STORAGE_PATH) + filename;
    std::cout << "[DedicatedThread] Sending file from DB: " << relativePath << " (path: " << fullPath << ")" << std::endl;
    
    // Kiểm tra file tồn tại TRƯỚC khi gửi header
    int fd = open(fullPath.c_str(), O_RDONLY);
    if (fd < 0) {
        std::cerr << "[DedicatedThread] File not found on disk, skipping: " << fullPath << " - " << strerror(errno) << std::endl;
        // KHÔNG gửi gì cả, skip file này
        return;
    }

    struct stat st;
    fstat(fd, &st);
    uint64_t fileSize = st.st_size;

    // File tồn tại, bây giờ mới gửi header
    // Send TYPE_FILE
    uint8_t type = TYPE_FILE;
    send(socketFd, &type, 1, 0);

    // Send name length
    uint32_t nameLen = htonl(relativePath.length());
    send(socketFd, &nameLen, 4, 0);

    // Send file size
    uint64_t netSize = htobe64(fileSize);
    send(socketFd, &netSize, 8, 0);

    // Send name
    send(socketFd, relativePath.c_str(), relativePath.length(), 0);

    // Send file data
    char buffer[65536];
    ssize_t n;
    uint64_t totalSent = 0;
    while ((n = read(fd, buffer, sizeof(buffer))) > 0) {
        send(socketFd, buffer, n, 0);
        totalSent += n;
    }

    close(fd);
    std::cout << "[DedicatedThread] File sent: " << relativePath << " (" << totalSent << " bytes)" << std::endl;
}

// Gửi directory đệ quy từ database
void DedicatedThread::sendDirectoryFromDb(int socketFd, long long folder_id, const std::string& relativePath, const std::string& username) {
    std::cout << "[DedicatedThread] Sending directory from DB: " << relativePath << " (id: " << folder_id << ")" << std::endl;
    
    // Send TYPE_DIR
    uint8_t type = TYPE_DIR;
    send(socketFd, &type, 1, 0);

    // Send name length
    uint32_t nameLen = htonl(relativePath.length());
    send(socketFd, &nameLen, 4, 0);

    // Send name
    send(socketFd, relativePath.c_str(), relativePath.length(), 0);

    // Lấy danh sách items trong folder từ database
    auto items = DBManager::getInstance().getItemsInFolder(folder_id, username);
    
    for (const auto& item : items) {
        std::string newRelPath = relativePath + "/" + item.name;
        
        if (item.is_folder) {
            // Đệ quy cho subfolder
            sendDirectoryFromDb(socketFd, item.file_id, newRelPath, username);
        } else {
            // Gửi file - tên file trong storage = item.name
            sendFileFromDb(socketFd, item.name, newRelPath);
        }
    }
}

void DedicatedThread::handleFolderDownload(int socketFd, long long folder_id, const std::string& folderName, const std::string& username, WorkerThread* workerRef) {
    std::cout << "[DedicatedThread] Downloading folder from DB: id=" << folder_id << ", name=" << folderName << std::endl;
    
    ThreadMonitor::getInstance().reportDedicatedThreadStart();
    
    // Disable Nagle's algorithm for immediate sending
    int flag = 1;
    setsockopt(socketFd, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int));

    // Gửi folder đệ quy từ database
    sendDirectoryFromDb(socketFd, folder_id, folderName, username);

    // Send TYPE_END
    uint8_t type = TYPE_END;
    ssize_t sent = send(socketFd, &type, 1, 0);
    std::cout << "[DedicatedThread] Sent TYPE_END (bytes sent: " << sent << ")" << std::endl;

    std::cout << "[DedicatedThread] Folder download completed: " << folderName << std::endl;
    
    ThreadMonitor::getInstance().reportDedicatedThreadEnd();
    
    // Trả socket về cho worker thay vì close
    if (workerRef) {
        ClientSession restoredSession;
        restoredSession.socketFd = socketFd;
        restoredSession.username = username;
        restoredSession.isAuthenticated = true;
        
        workerRef->addClient(socketFd, restoredSession);
        std::cout << "[DedicatedThread] Socket " << socketFd << " returned to worker (user: " << username << ")" << std::endl;
    }
}

void DedicatedThread::handleDownload(int socketFd, std::string filename, std::string username, WorkerThread* workerRef) {
    ThreadMonitor::getInstance().reportDedicatedThreadStart();

    std::string path = std::string(STORAGE_PATH) + filename;

    std::ifstream infile(path, std::ios::binary | std::ios::ate);
    if (!infile.is_open()) {
        std::string err = std::string(CODE_FAIL) + " File not found on server\n";
        send(socketFd, err.c_str(), err.length(), 0);
        
        if (workerRef) {
            ClientSession restoredSession;
            restoredSession.socketFd = socketFd;
            restoredSession.username = username;
            restoredSession.isAuthenticated = true;
            workerRef->addClient(socketFd, restoredSession);
        }
        ThreadMonitor::getInstance().reportDedicatedThreadEnd();
        return;
    }

    long filesize = infile.tellg();
    infile.seekg(0, std::ios::beg);

    std::string msg = std::string(CODE_DATA_OPEN) + " " + std::to_string(filesize) + "\n";
    send(socketFd, msg.c_str(), msg.length(), 0);

    char buffer[BUFFER_SIZE];
    long totalSent = 0;
    const long ACK_GROUP_SIZE = 1048576;
    long bytesSinceLastAck = 0;
    
    while (!infile.eof()) {
        infile.read(buffer, BUFFER_SIZE);
        int bytesRead = infile.gcount();
        if (bytesRead > 0) {
            send(socketFd, buffer, bytesRead, 0);
            totalSent += bytesRead;
            bytesSinceLastAck += bytesRead;
            
            if (bytesSinceLastAck >= ACK_GROUP_SIZE && totalSent < filesize) {
                char ackBuf[256];
                fd_set readfds;
                struct timeval tv;
                FD_ZERO(&readfds);
                FD_SET(socketFd, &readfds);
                tv.tv_sec = 3;
                tv.tv_usec = 0;
                
                int ret = select(socketFd + 1, &readfds, NULL, NULL, &tv);
                if (ret > 0) {
                    int n = recv(socketFd, ackBuf, sizeof(ackBuf) - 1, 0);
                    if (n > 0) {
                        ackBuf[n] = '\0';
                    }
                }
                bytesSinceLastAck = 0;
            }
        }
    }
    
    infile.close();

    msg = std::string(CODE_TRANSFER_COMPLETE) + " Download success\n";
    send(socketFd, msg.c_str(), msg.length(), 0);

    ThreadMonitor::getInstance().reportBytesTransferred(filesize);
    ThreadMonitor::getInstance().reportDedicatedThreadEnd();
    
    if (workerRef) {
        ClientSession restoredSession;
        restoredSession.socketFd = socketFd;
        restoredSession.username = username;
        restoredSession.isAuthenticated = true;
        
        workerRef->addClient(socketFd, restoredSession);
        std::cout << "[Dedicated] Socket " << socketFd << " returned with session (user: " << username << ")" << std::endl;
    }
}