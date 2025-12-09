#ifndef THREAD_MONITOR_H
#define THREAD_MONITOR_H

#include "server_config.h"
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <vector>
#include <map>
#include <set>
#include <iostream>

// Forward declaration
class WorkerThread;

// Cấu trúc lưu thông tin stats của mỗi loại thread
struct ThreadStats {
    std::atomic<int> activeWorkerThreads{0};      // Số WorkerThread đang chạy
    std::atomic<int> activeDedicatedThreads{0};   // Số DedicatedThread đang chạy
    std::atomic<int> totalConnections{0};         // Tổng số connections đang xử lý
    std::atomic<long long> totalBytesTransferred{0}; // Tổng bytes đã transfer
};

class ThreadMonitor {
public:
    static ThreadMonitor& getInstance() {
        static ThreadMonitor instance;
        return instance;
    }

    // Khởi động monitor thread
    void start();
    
    // Dừng monitor thread
    void stop();

    // Cập nhật stats từ các thread
    void reportWorkerThreadStart();
    void reportWorkerThreadEnd();
    void reportDedicatedThreadStart();
    void reportDedicatedThreadEnd();
    void reportConnectionCount(std::thread::id workerId, int count);  // Báo cáo từ từng worker
    void reportBytesTransferred(long long bytes);

    // Lấy thông tin stats (cho admin console)
    void printStats();
    
    bool canCreateDedicatedThread();  // Kiểm tra còn slot để tạo DedicatedThread không
    
    // Quản lý Worker Thread Pool
    void registerWorkerThread(WorkerThread* worker, std::thread::id threadId);
    void unregisterWorkerThread(std::thread::id threadId);
    int getActiveWorkerCount() const;
    
    // Quản lý Dedicated Thread Pool (cleanup finished threads)
    void registerDedicatedThread(std::thread::id threadId, std::thread&& thread);
    void cleanupFinishedThreads();

private:
    ThreadMonitor() = default;
    ~ThreadMonitor() { stop(); }
    
    ThreadMonitor(const ThreadMonitor&) = delete;
    ThreadMonitor& operator=(const ThreadMonitor&) = delete;

    void monitorLoop(); // Vòng lặp chính của monitor

    std::thread monitorThread;
    std::atomic<bool> running{false};
    ThreadStats stats;
    
    // Worker Thread Pool
    std::mutex poolMutex;
    std::map<std::thread::id, WorkerThread*> workerPool;
    std::map<std::thread::id, int> workerConnections;  // Track connections per worker
    
    // Dedicated Thread Pool (cho cleanup)
    std::mutex dedicatedMutex;
    std::vector<std::thread> dedicatedThreads;
    std::set<std::thread::id> finishedThreadIds;
    
    // Ngưỡng cảnh báo - Sử dụng ServerConfig
    static constexpr int MAX_DEDICATED_THREADS = ServerConfig::MAX_DEDICATED_THREADS;
    static constexpr int FIXED_WORKER_THREADS = ServerConfig::FIXED_WORKER_THREADS;
};

#endif // THREAD_MONITOR_H
