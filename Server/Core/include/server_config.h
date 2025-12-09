#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

// Cấu hình tập trung cho toàn bộ server
struct ServerConfig {
    // ============ NETWORK CONFIG ============
    static constexpr int SERVER_PORT = 8080;
    static constexpr int LISTEN_BACKLOG = 10;  // Số kết nối chờ trong queue
    
    // ============ WORKER THREAD CONFIG ============
    static constexpr int FIXED_WORKER_THREADS = 10;  // Số worker threads cố định trong pool
    
    // ============ DEDICATED THREAD CONFIG ============
    // File I/O threads - điều chỉnh theo:
    // - RAM: 8GB → 100-200 threads OK
    // - HDD: 50-100 (tránh thrashing)
    // - SSD: 100-300 (tốc độ cao hơn)
    static constexpr int MAX_DEDICATED_THREADS = 100;  // Upload/Download đồng thời
    
    // ============ MONITOR CONFIG ============
    static constexpr int MONITOR_INTERVAL_SECONDS = 5;     // In stats mỗi 5 giây
    static constexpr int CLEANUP_INTERVAL_SECONDS = 5;     // Cleanup threads mỗi 5 giây
    
    // ============ LOAD BALANCING CONFIG ============
    // Không cần config - tự động chọn worker ít kết nối nhất
    
    // ============ DATABASE CONFIG ============
    static constexpr const char* DB_HOST = "localhost";
    static constexpr const char* DB_USER = "cuong";
    static constexpr const char* DB_PASS = "040424";
    static constexpr const char* DB_NAME = "file_management";
    static constexpr int DB_PORT = 3306;
    
    // ============ STORAGE CONFIG ============
    static constexpr const char* STORAGE_PATH = "/home/cuong/DuAnPMCSF/File_Management_App/Server/storage/";
    static constexpr long DEFAULT_USER_QUOTA = 1073741824;  // 1GB in bytes
    
    // ============ BUFFER CONFIG ============
    static constexpr int BUFFER_SIZE = 4096;  // 4KB buffer cho file I/O
};

#endif // SERVER_CONFIG_H
