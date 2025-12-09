#include "thread_monitor.h"
#include "thread_manager.h"

void ThreadMonitor::start() {
    if (running.load()) {
        std::cout << "[Monitor] Already running!" << std::endl;
        return;
    }
    
    running.store(true);
    monitorThread = std::thread(&ThreadMonitor::monitorLoop, this);
    std::cout << "[Monitor] Thread started" << std::endl;
}

void ThreadMonitor::stop() {
    if (!running.load()) return;
    
    running.store(false);
    if (monitorThread.joinable()) {
        monitorThread.join();
    }
    std::cout << "[Monitor] Thread stopped" << std::endl;
}

void ThreadMonitor::reportWorkerThreadStart() {
    stats.activeWorkerThreads++;
    std::cout << "[Monitor] Worker thread started. Active: " 
              << stats.activeWorkerThreads.load() << std::endl;
}

void ThreadMonitor::reportWorkerThreadEnd() {
    stats.activeWorkerThreads--;
    std::cout << "[Monitor] Worker thread ended. Active: " 
              << stats.activeWorkerThreads.load() << std::endl;
}

void ThreadMonitor::reportDedicatedThreadStart() {
    stats.activeDedicatedThreads++;
}

void ThreadMonitor::reportDedicatedThreadEnd() {
    stats.activeDedicatedThreads--;
    
    // Đánh dấu thread hiện tại đã finished
    std::lock_guard<std::mutex> lock(dedicatedMutex);
    finishedThreadIds.insert(std::this_thread::get_id());
}

void ThreadMonitor::reportConnectionCount(std::thread::id workerId, int count) {
    std::lock_guard<std::mutex> lock(poolMutex);
    workerConnections[workerId] = count;
    
    // Tính tổng connections từ tất cả workers
    int total = 0;
    std::cout << "[Monitor] Connection report - Worker " << workerId << ": " << count << " connections" << std::endl;
    for (const auto& pair : workerConnections) {
        std::cout << "  Worker " << pair.first << " has " << pair.second << " connections" << std::endl;
        total += pair.second;
    }
    std::cout << "[Monitor] Total connections: " << total << std::endl;
    stats.totalConnections.store(total);
}

void ThreadMonitor::reportBytesTransferred(long long bytes) {
    stats.totalBytesTransferred += bytes;
}

void ThreadMonitor::printStats() {
    std::cout << "\n========== SYSTEM STATS ==========\n";
    std::cout << "Worker Threads:     " << stats.activeWorkerThreads.load() << "\n";
    std::cout << "Dedicated Threads:  " << stats.activeDedicatedThreads.load() << "\n";
    std::cout << "Total Connections:  " << stats.totalConnections.load() << "\n";
    std::cout << "Bytes Transferred:  " << stats.totalBytesTransferred.load() 
              << " bytes (" << (stats.totalBytesTransferred.load() / 1024.0 / 1024.0) << " MB)\n";
    std::cout << "==================================\n" << std::endl;
}

bool ThreadMonitor::canCreateDedicatedThread() {
    int dedicated = stats.activeDedicatedThreads.load();
    
    if (dedicated >= MAX_DEDICATED_THREADS) {
        std::cout << "[Monitor]  Cannot create DedicatedThread: limit reached (" 
                  << dedicated << "/" << MAX_DEDICATED_THREADS << ")" << std::endl;
        return false;
    }
    
    return true;
}

void ThreadMonitor::monitorLoop() {
    using namespace std::chrono;
    
    auto lastPrintTime = steady_clock::now();
    const int PRINT_INTERVAL_SECONDS = 30; // In stats mỗi 30 giây
    
    while (running.load()) {
        // Sleep 5 giây
        std::this_thread::sleep_for(seconds(5));
        
        auto now = steady_clock::now();
        auto elapsed = duration_cast<seconds>(now - lastPrintTime).count();
        
        // In stats định kỳ
        if (elapsed >= PRINT_INTERVAL_SECONDS) {
            printStats();
            lastPrintTime = now;
        }
        
        // Cleanup các dedicated thread đã hoàn thành
        cleanupFinishedThreads();
    }
}

// Quản lý Worker Pool
void ThreadMonitor::registerWorkerThread(WorkerThread* worker, std::thread::id threadId) {
    std::lock_guard<std::mutex> lock(poolMutex);
    workerPool[threadId] = worker;
    workerConnections[threadId] = 0;  // Khởi tạo count = 0 cho worker mới
    reportWorkerThreadStart();
    std::cout << "[Monitor] Worker thread registered. ID: " << threadId << std::endl;
}

void ThreadMonitor::unregisterWorkerThread(std::thread::id threadId) {
    std::lock_guard<std::mutex> lock(poolMutex);
    workerPool.erase(threadId);
    workerConnections.erase(threadId);  // Xóa entry khỏi map
    reportWorkerThreadEnd();
    std::cout << "[Monitor] Worker thread unregistered. ID: " << threadId << std::endl;
}

int ThreadMonitor::getActiveWorkerCount() const {
    return stats.activeWorkerThreads.load();
}

// Quản lý Dedicated Thread Pool
void ThreadMonitor::registerDedicatedThread(std::thread::id threadId, std::thread&& thread) {
    std::lock_guard<std::mutex> lock(dedicatedMutex);
    dedicatedThreads.push_back(std::move(thread));
    std::cout << "[Monitor] Dedicated thread registered. ID: " << threadId 
              << " (Total active: " << stats.activeDedicatedThreads.load() << ")" << std::endl;
}

void ThreadMonitor::cleanupFinishedThreads() {
    std::lock_guard<std::mutex> lock(dedicatedMutex);
    
    // Duyệt ngược để có thể erase an toàn
    for (auto it = dedicatedThreads.begin(); it != dedicatedThreads.end(); ) {
        // Kiểm tra thread có joinable không (nếu finished thì còn joinable)
        if (it->joinable()) {
            // Thử join với timeout = 0 (không block)
            // Nếu thread đã kết thúc, join sẽ thành công ngay
            std::thread::id tid = it->get_id();
            
            // Tạm thời detach để kiểm tra - nếu đã finished sẽ cleanup
            // (C++ không có cách kiểm tra thread finished mà không block)
            // Nên ta dùng cơ chế khác: theo dõi qua reportDedicatedThreadEnd()
            
            // Nếu thread ID nằm trong finishedThreadIds, join và xóa
            if (finishedThreadIds.count(tid) > 0) {
                it->join();
                std::cout << "[Monitor] Cleaned up finished dedicated thread: " << tid << std::endl;
                finishedThreadIds.erase(tid);
                it = dedicatedThreads.erase(it);
            } else {
                ++it;
            }
        } else {
            // Thread không joinable (có thể đã detach hoặc moved), xóa luôn
            it = dedicatedThreads.erase(it);
        }
    }
}
