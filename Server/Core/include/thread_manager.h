#ifndef THREAD_MANAGER_H
#define THREAD_MANAGER_H

#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <map>
#include <algorithm>
#include "server.h"

// Class xử lý đa nhiệm (Worker)
class WorkerThread {
public:
    WorkerThread();
    void addClient(int socketFd);
    void addClient(int socketFd, const ClientSession& session);  // Khôi phục session
    void run(); 
    void stop();
    int getConnectionCount() const { return client_sockets.size(); }

private:
    void handleClientMessage(int fd);
    
    // CẬP NHẬT: Thêm tham số closeSocket (mặc định là true)
    // Nếu false: Chỉ ngừng theo dõi, không đóng socket (để chuyển cho thread khác)
    void removeClient(int fd, bool closeSocket = true);

    std::vector<int> client_sockets;
    std::map<int, ClientSession> sessions;
    std::mutex mtx;
    std::atomic<bool> running;
    int epoll_fd;  // epoll file descriptor
    std::thread::id myThreadId;  // Lưu thread ID thực tế của worker này
};

// Class xử lý riêng (Dedicated)
class DedicatedThread {
public:
    void handleUpload(int socketFd, std::string filename, long filesize, std::string username, long long parent_id, WorkerThread* workerRef);
    void handleDownload(int socketFd, std::string filename, std::string username, WorkerThread* workerRef);
    void handleFolderDownload(int socketFd, const std::string& folderName, const std::string& username);
    
private:
    void sendFile(int socketFd, const std::string& fullPath, const std::string& relativePath);
    void sendDirectory(int socketFd, const std::string& basePath, const std::string& relativePath);
};

// Class chấp nhận kết nối (Acceptor)
class AcceptorThread {
public:
    AcceptorThread(int port);
    void run();
    void stop();
private:
    int server_fd;
    int port;
    std::atomic<bool> running{true};
    
    // Worker Thread Pool (fixed size)
    std::vector<std::unique_ptr<WorkerThread>> workerPool;
    std::vector<std::thread> workerThreads;
    std::mutex poolMutex;
    
    void createWorkerPool();  // Tạo pool cố định
    WorkerThread* selectLeastLoadedWorker();  // Chọn worker ít kết nối nhất
};

#endif // THREAD_MANAGER_H