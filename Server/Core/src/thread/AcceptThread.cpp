#include "../../include/thread_manager.h"
#include "../../include/thread_monitor.h"
#include "../../include/server_config.h"
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <memory>

AcceptorThread::AcceptorThread(int p) : port(p) {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    // Cấu hình để tái sử dụng cổng ngay khi tắt server
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, ServerConfig::LISTEN_BACKLOG) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    
    createWorkerPool();
}

void AcceptorThread::createWorkerPool() {
    std::cout << "[Acceptor] Creating fixed worker pool (" 
              << ServerConfig::FIXED_WORKER_THREADS << " threads)..." << std::endl;
    
    for (int i = 0; i < ServerConfig::FIXED_WORKER_THREADS; i++) {
        auto worker = std::make_unique<WorkerThread>();
        std::thread workerThread([w = worker.get()]() { w->run(); });
        
        workerPool.push_back(std::move(worker));
        workerThreads.push_back(std::move(workerThread));
    }
    
    std::cout << "[Acceptor] Worker pool created with " 
              << workerPool.size() << " threads" << std::endl;
}

WorkerThread* AcceptorThread::selectLeastLoadedWorker() {
    std::lock_guard<std::mutex> lock(poolMutex);
    
    if (workerPool.empty()) return nullptr;
    
    // Tìm worker có ít connections nhất
    WorkerThread* leastLoaded = workerPool[0].get();
    int minConnections = leastLoaded->getConnectionCount();
    int selectedIndex = 0;
    
    for (size_t i = 1; i < workerPool.size(); i++) {
        int count = workerPool[i]->getConnectionCount();
        if (count < minConnections) {
            minConnections = count;
            leastLoaded = workerPool[i].get();
            selectedIndex = i;
        }
    }
    
    std::cout << "[Acceptor] Selected worker #" << selectedIndex 
              << " (load: " << minConnections << " connections)" << std::endl;
    
    return leastLoaded;
}

void AcceptorThread::run() {
    std::cout << "[Acceptor] Listening on port " << port << "..." << std::endl;
    
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    while (running) {
        // Chấp nhận kết nối từ Client
        int new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (new_socket < 0) {
            if (running) {
                perror("Accept error");
            }
            continue;
        }

        // Lấy địa chỉ IP của client
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &address.sin_addr, client_ip, INET_ADDRSTRLEN);
        int client_port = ntohs(address.sin_port);
        
        std::cout << "[Acceptor] New connection from " << client_ip 
                  << ":" << client_port << " (FD: " << new_socket << ")" << std::endl;
        
        // Chọn WorkerThread ít kết nối nhất
        WorkerThread* worker = selectLeastLoadedWorker();
        if (worker) {
            worker->addClient(new_socket);
        } else {
            std::cerr << "[Acceptor] ERROR: No worker available!" << std::endl;
            close(new_socket);
        }
    }
}

void AcceptorThread::stop() {
    running = false;
    
    // Đóng server socket để thoát khỏi accept()
    if (server_fd >= 0) {
        close(server_fd);
    }
    
    // Dừng tất cả workers
    for (auto& worker : workerPool) {
        worker->stop();
    }
    
    // Join tất cả worker threads
    for (auto& t : workerThreads) {
        if (t.joinable()) {
            t.join();
        }
    }
}