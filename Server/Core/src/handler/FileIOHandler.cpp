#include "../../include/request_handler.h"
#include "../../include/db_manager.h"
#include "../../include/server_config.h"
#include "../../../../Common/Protocol.h"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

std::string FileIOHandler::handleQuotaCheck(const ClientSession& session, long filesize) {
    std::cout << "[FileIOHandler::QUOTA_CHECK] User: " << session.username << ", File size: " << filesize << " bytes" << std::endl;
    
    if (!session.isAuthenticated) {
        std::cout << "[FileIOHandler::QUOTA_CHECK] User not authenticated" << std::endl;
        return std::string(CODE_LOGIN_FAIL) + " Not logged in\n";
    }
    
    std::string userPath = std::string(ServerConfig::STORAGE_PATH);
    if (!userPath.empty() && userPath.back() == '/') {
        userPath.pop_back();
    }
    
    if (!fs::exists(userPath)) {
        try {
            fs::create_directories(userPath);
        } catch (...) {
            return std::string(CODE_FAIL) + " Server Storage Error\n";
        }
    }

    long used = DBManager::getInstance().getStorageUsed(session.username);
    long limit = 1073741824;
    
    std::cout << "[FileIOHandler::QUOTA_CHECK] Used: " << used << " bytes, Limit: " << limit << " bytes" << std::endl;
    
    if (used + filesize > limit) {
        std::cout << "[FileIOHandler::QUOTA_CHECK] QUOTA EXCEEDED" << std::endl;
        return std::string(CODE_FAIL) + " Quota exceeded\n";
    }
    
    std::cout << "[FileIOHandler::QUOTA_CHECK] QUOTA OK" << std::endl;
    return std::string(CODE_OK) + " Quota OK\n";
}

bool FileIOHandler::checkDownloadPermission(const ClientSession& session, const std::string& filename) {
    std::cout << "[FileIOHandler::CHECK_PERMISSION] User: " << session.username << ", File: " << filename << std::endl;
    
    if (!session.isAuthenticated) {
        std::cout << "[FileIOHandler::CHECK_PERMISSION] User not authenticated" << std::endl;
        return false;
    }
    
    std::string filePath = std::string(ServerConfig::STORAGE_PATH) + filename;
    
    if (!fs::exists(filePath)) {
        std::cerr << "[FileIOHandler] File not found: " << filePath << std::endl;
        return false;
    }
    
    DBManager& dbManager = DBManager::getInstance();
    if (!dbManager.connect()) {
        std::cerr << "[FileIOHandler] Database connection failed!" << std::endl;
        return false;
    }
    
    std::string owner = dbManager.getFileOwner(filename);
    
    if (owner.empty()) {
        std::cout << "[FileIOHandler::CHECK_PERMISSION] No owner found, allowing access" << std::endl;
        return true;
    }
    
    if (owner == session.username) {
        std::cout << "[FileIOHandler::CHECK_PERMISSION] User is owner, access GRANTED" << std::endl;
        return true;
    }
    
    bool isShared = dbManager.isFileSharedWithUser(filename, session.username);
    
    if (isShared) {
        std::cout << "[FileIOHandler] File '" << filename << "' is shared with user '" << session.username << "'" << std::endl;
        return true;
    }
    
    std::cerr << "[FileIOHandler] Permission denied for user '" << session.username << "' to access '" << filename << "'" << std::endl;
    return false;
}