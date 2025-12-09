#include "../../include/request_handler.h"
#include "../../include/db_manager.h"
#include "../../../../Common/Protocol.h"

std::string CmdHandler::handleList(const ClientSession& session) {
    if (!session.isAuthenticated) {
        return std::string(CODE_FAIL) + " Please login first\n";
    }

    auto files = DBManager::getInstance().getFiles(session.username);
    
    if (files.empty()) {
        return "210 Empty folder\n";
    }

    std::string response = "";
    // Format chuẩn: Tên|Size|Người sở hữu
    for (const auto& f : files) {
        response += f.name + "|" + std::to_string(f.size) + "|" + f.owner + "\n";
    }
    return response;
}

std::string CmdHandler::handleListShared(const ClientSession& session) {
    if (!session.isAuthenticated) {
        return std::string(CODE_FAIL) + " Please login first\n";
    }

    auto files = DBManager::getInstance().getSharedFiles(session.username);
    
    if (files.empty()) {
        return "210 No shared files\n";
    }

    std::string response = "";
    // Format: Tên|Size|Người share
    for (const auto& f : files) {
        response += f.name + "|" + std::to_string(f.size) + "|" + f.owner + "\n";
    }
    return response;
}

std::string CmdHandler::handleSearch(const ClientSession& session, const std::string& keyword) {
    if (!session.isAuthenticated) return std::string(CODE_FAIL) + " Please login first\n";

    // TODO: Gọi DBManager::searchFiles(keyword)
    // Code giả lập:
    return "Result_File_1.txt|1024|admin\n";
}

std::string CmdHandler::handleShare(const ClientSession& session, const std::string& filename, const std::string& targetUser) {
    if (!session.isAuthenticated) {
        return std::string(CODE_FAIL) + " Please login first\n";
    }

    // Gọi DBManager để share file
    bool success = DBManager::getInstance().shareFile(filename, session.username, targetUser);
    
    if (success) {
        return std::string(CODE_OK) + " File '" + filename + "' shared with " + targetUser + "\n";
    } else {
        return std::string(CODE_FAIL) + " Failed to share file. Check if file exists and target user is valid\n";
    }
}

std::string CmdHandler::handleDelete(const ClientSession& session, const std::string& filename) {
    if (!session.isAuthenticated) {
        return std::string(CODE_FAIL) + " Please login first\n";
    }

    // Gọi DBManager để xóa file (chỉ owner mới được xóa)
    bool success = DBManager::getInstance().deleteFile(filename, session.username);
    
    if (success) {
        return std::string(CODE_OK) + " File '" + filename + "' deleted successfully\n";
    } else {
        return std::string(CODE_FAIL) + " Failed to delete file. You must be the owner to delete this file\n";
    }
}