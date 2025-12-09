#include "../../include/thread_manager.h"
#include "../../include/db_manager.h"
#include "../../include/thread_monitor.h"
#include "../../include/server_config.h"
#include "../../../../Common/Protocol.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <unistd.h>
#include <sys/socket.h>

#define BUFFER_SIZE ServerConfig::BUFFER_SIZE
#define STORAGE_PATH ServerConfig::STORAGE_PATH

// --- XỞ LÝ UPLOAD (Client gửi lên Server) ---
void DedicatedThread::handleUpload(int socketFd, std::string filename, long filesize, std::string username, WorkerThread* workerRef) {
    ThreadMonitor::getInstance().reportDedicatedThreadStart();
    std::cout << "[Dedicated] Starting Upload: " << filename << " (" << filesize << " bytes) by " << username << std::endl;

    // 1. Tạo file trong thư mục 'storage'
    std::string path = std::string(STORAGE_PATH) + filename;
    std::ofstream outfile(path, std::ios::binary);
    
    if (!outfile.is_open()) {
        std::string err = std::string(CODE_FAIL) + " Cannot create file on server\n";
        send(socketFd, err.c_str(), err.length(), 0);
        close(socketFd);
        return;
    }
    
    // 2. Gửi tín hiệu sẵn sàng (150 Data connection open)
    std::string msg = std::string(CODE_DATA_OPEN) + " Ready to receive data\n";
    send(socketFd, msg.c_str(), msg.length(), 0);

    // 3. Vòng lặp nhận dữ liệu
    char buffer[BUFFER_SIZE];
    long totalReceived = 0;
    
    while (totalReceived < filesize) {
        int bytesRead = read(socketFd, buffer, BUFFER_SIZE);
        if (bytesRead <= 0) break; // Ngắt kết nối hoặc lỗi

        outfile.write(buffer, bytesRead);
        totalReceived += bytesRead;
    }

    outfile.close();
    std::cout << "[Dedicated] Upload Finished: " << filename << std::endl;

    // Lưu vào database
    bool saved = DBManager::getInstance().addFile(filename, totalReceived, username);
    if (saved) {
        std::cout << "[Dedicated] File metadata saved to database" << std::endl;
    } else {
        std::cerr << "[Dedicated] Failed to save file metadata to database" << std::endl;
    }

    // 4. Báo hoàn tất (226 Transfer complete)
    msg = std::string(CODE_TRANSFER_COMPLETE) + " Upload success\n";
    send(socketFd, msg.c_str(), msg.length(), 0);
    
    ThreadMonitor::getInstance().reportBytesTransferred(filesize);
    ThreadMonitor::getInstance().reportDedicatedThreadEnd();
    
    // KHÔNG đóng socket - trả lại cho WorkerThread
    std::cout << "[Dedicated] Upload complete, returning socket to WorkerThread..." << std::endl;
    
    // Trả socket về WorkerThread và khôi phục session
    if (workerRef) {
        ClientSession restoredSession;
        restoredSession.socketFd = socketFd;
        restoredSession.username = username;
        restoredSession.isAuthenticated = true;
        
        workerRef->addClient(socketFd, restoredSession);
        std::cout << "[Dedicated] Socket " << socketFd << " returned with session (user: " << username << ")" << std::endl;
    }
}

// --- XỞ LÝ DOWNLOAD (Server gửi xuống Client) ---
void DedicatedThread::handleDownload(int socketFd, std::string filename, std::string username, WorkerThread* workerRef) {
    ThreadMonitor::getInstance().reportDedicatedThreadStart();
    std::cout << "[Dedicated] Starting Download: " << filename << std::endl;

    // Đường dẫn file trong thư mục storage (giống với upload)
    std::string path = std::string(STORAGE_PATH) + filename;

    // Mở file đọc binary
    std::ifstream infile(path, std::ios::binary | std::ios::ate);
    if (!infile.is_open()) {
        std::string err = std::string(CODE_FAIL) + " File not found on server\n";
        send(socketFd, err.c_str(), err.length(), 0);
        
        // Trả socket về WorkerThread ngay cả khi lỗi
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

    // Lấy kích thước file
    long filesize = infile.tellg();
    infile.seekg(0, std::ios::beg);

    // 1. Gửi thông báo bắt đầu kèm kích thước (150 <filesize>)
    std::string msg = std::string(CODE_DATA_OPEN) + " " + std::to_string(filesize) + "\n";
    send(socketFd, msg.c_str(), msg.length(), 0);

    // 2. Vòng lặp đọc file và gửi
    char buffer[BUFFER_SIZE];
    while (!infile.eof()) {
        infile.read(buffer, BUFFER_SIZE);
        int bytesRead = infile.gcount();
        if (bytesRead > 0) {
            send(socketFd, buffer, bytesRead, 0);
        }
    }
    
    infile.close();
    std::cout << "[Dedicated] Download Finished: " << filename << std::endl;

    // 4. Báo hoàn tất (226 Transfer complete)
    msg = std::string(CODE_TRANSFER_COMPLETE) + " Download success\n";
    send(socketFd, msg.c_str(), msg.length(), 0);

    ThreadMonitor::getInstance().reportBytesTransferred(filesize);
    ThreadMonitor::getInstance().reportDedicatedThreadEnd();
    
    // KHÔNG đóng socket - trả lại cho WorkerThread
    std::cout << "[Dedicated] Download complete, returning socket to WorkerThread..." << std::endl;
    
    // Trả socket về WorkerThread và khôi phục session
    if (workerRef) {
        ClientSession restoredSession;
        restoredSession.socketFd = socketFd;
        restoredSession.username = username;
        restoredSession.isAuthenticated = true;
        
        workerRef->addClient(socketFd, restoredSession);
        std::cout << "[Dedicated] Socket " << socketFd << " returned with session (user: " << username << ")" << std::endl;
    }
}