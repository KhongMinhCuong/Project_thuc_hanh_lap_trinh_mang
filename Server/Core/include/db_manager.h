#ifndef DB_MANAGER_H
#define DB_MANAGER_H

#include <string>
#include <vector>
#include <mysql/mysql.h>

// Cấu trúc mô phỏng thông tin File
struct FileRecord {
    std::string name;
    long size;
    std::string owner;
};

class DBManager {
public:
    static DBManager& getInstance() {
        static DBManager instance;
        return instance;
    }

    bool connect();
    void disconnect();
    bool checkUser(std::string user, std::string pass);
    std::vector<FileRecord> getFiles(std::string username);
    std::vector<FileRecord> getSharedFiles(std::string username);
    bool addFile(std::string filename, long filesize, std::string owner);
    long getStorageUsed(std::string username);
    bool shareFile(std::string filename, std::string ownerUsername, std::string targetUsername);
    bool deleteFile(std::string filename, std::string username);

private:
    DBManager() : conn(nullptr) {} 
    ~DBManager() { disconnect(); }
    MYSQL* conn;
};

#endif