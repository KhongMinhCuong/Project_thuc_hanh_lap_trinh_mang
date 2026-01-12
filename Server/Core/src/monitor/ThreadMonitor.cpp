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
    
    std::lock_guard<std::mutex> lock(dedicatedMutex);
    finishedThreadIds.insert(std::this_thread::get_id());
}

void ThreadMonitor::reportConnectionCount(std::thread::id workerId, int count) {
    std::lock_guard<std::mutex> lock(poolMutex);
    
    // Only log if connection count changed
    bool changed = false;
    if (workerConnections.find(workerId) == workerConnections.end() || workerConnections[workerId] != count) {
        changed = true;
    }
    
    workerConnections[workerId] = count;
    
    int total = 0;
    for (const auto& pair : workerConnections) {
        total += pair.second;
    }
    
    // Only print when connection count actually changes
    if (changed) {
        std::cout << "[Monitor] Connection change - Worker " << workerId << ": " << count << " connections (Total: " << total << ")" << std::endl;
    }
    
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
    const int PRINT_INTERVAL_SECONDS = 300; // Print stats every 5 minutes instead of 30 seconds
    
    while (running.load()) {
        std::this_thread::sleep_for(seconds(5));
        
        auto now = steady_clock::now();
        auto elapsed = duration_cast<seconds>(now - lastPrintTime).count();
        
        if (elapsed >= PRINT_INTERVAL_SECONDS) {
            printStats();
            lastPrintTime = now;
        }
        
        cleanupFinishedThreads();
    }
}

void ThreadMonitor::registerWorkerThread(WorkerThread* worker, std::thread::id threadId) {
    std::lock_guard<std::mutex> lock(poolMutex);
    workerPool[threadId] = worker;
    workerConnections[threadId] = 0;
    reportWorkerThreadStart();
    std::cout << "[Monitor] Worker thread registered. ID: " << threadId << std::endl;
}

void ThreadMonitor::unregisterWorkerThread(std::thread::id threadId) {
    std::lock_guard<std::mutex> lock(poolMutex);
    workerPool.erase(threadId);
    workerConnections.erase(threadId);
    reportWorkerThreadEnd();
    std::cout << "[Monitor] Worker thread unregistered. ID: " << threadId << std::endl;
}

int ThreadMonitor::getActiveWorkerCount() const {
    return stats.activeWorkerThreads.load();
}

void ThreadMonitor::registerDedicatedThread(std::thread::id threadId, std::thread&& thread) {
    std::lock_guard<std::mutex> lock(dedicatedMutex);
    dedicatedThreads.push_back(std::move(thread));
    std::cout << "[Monitor] Dedicated thread registered. ID: " << threadId 
              << " (Total active: " << stats.activeDedicatedThreads.load() << ")" << std::endl;
}

void ThreadMonitor::cleanupFinishedThreads() {
    std::lock_guard<std::mutex> lock(dedicatedMutex);
    
    for (auto it = dedicatedThreads.begin(); it != dedicatedThreads.end(); ) {
        if (it->joinable()) {
            std::thread::id tid = it->get_id();
            
            if (finishedThreadIds.count(tid) > 0) {
                it->join();
                std::cout << "[Monitor] Cleaned up finished dedicated thread: " << tid << std::endl;
                finishedThreadIds.erase(tid);
                it = dedicatedThreads.erase(it);
            } else {
                ++it;
            }
        } else {
            it = dedicatedThreads.erase(it);
        }
    }
}
