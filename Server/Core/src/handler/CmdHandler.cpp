#include "../../include/request_handler.h"
#include "../../include/db_manager.h"
#include "../../../../Common/Protocol.h" // Đảm bảo đường dẫn này đúng với project của bạn
#include <iostream>
#include <sstream>
#include <vector>

// ======================================================
// CÁC HÀM QUẢN LÝ FILE CƠ BẢN (List, Search, Delete...)
// ======================================================

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

    // Code giả lập kết quả search
    // Thực tế: gọi DBManager::getInstance().searchFiles(keyword, session.username);
    return "Result_File_1.txt|1024|admin\n";
}

std::string CmdHandler::handleShare(const ClientSession& session, const std::string& filename, const std::string& targetUser) {
    if (!session.isAuthenticated) {
        return std::string(CODE_FAIL) + " Please login first\n";
    }

    // Gọi DBManager để share file lẻ
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

// ======================================================
// LOGIC TẠO FOLDER (MKDIR)
// ======================================================

std::string CmdHandler::handleCreateFolder(const ClientSession& session, 
                                           const std::string& folderName,
                                           long long parent_id) {
    if (!session.isAuthenticated) {
        return std::string(CODE_FAIL) + " Please login first\n";
    }
    
    // 1. Validate tên folder
    if (folderName.empty()) {
        return std::string(CODE_FAIL) + " Folder name cannot be empty\n";
    }
    
    if (folderName.find('/') != std::string::npos || 
        folderName.find('\\') != std::string::npos ||
        folderName.find('\0') != std::string::npos) {
        return std::string(CODE_FAIL) + " Invalid folder name\n";
    }
    
    // 2. Kiểm tra folder cha có tồn tại và thuộc quyền sở hữu không (trừ trường hợp root)
    if (parent_id != -1 && parent_id != 1) { // Giả sử 1 là root mặc định
        FileRecordEx parent = DBManager::getInstance().getFileInfo(parent_id);
        
        if (parent.file_id == -1) {
            return std::string(CODE_FAIL) + " Parent folder not found\n";
        }
        
        if (!parent.is_folder) {
            return std::string(CODE_FAIL) + " Parent is not a folder\n";
        }
        
        // Chỉ owner mới được tạo con trong folder của mình (hoặc phải check thêm quyền Write nếu là folder được share)
        if (parent.owner != session.username) {
            return std::string(CODE_FAIL) + " Permission denied\n";
        }
    }
    
    // 3. Kiểm tra trùng tên trong cùng thư mục cha
    auto items = DBManager::getInstance().getItemsInFolder(parent_id, session.username);
    for (const auto& item : items) {
        if (item.name == folderName && item.is_folder) {
            return std::string(CODE_FAIL) + " Folder already exists\n";
        }
    }
    
    // 4. Tạo folder trong DB
    long long new_folder_id = DBManager::getInstance().createFolder(
        folderName,
        parent_id,
        session.username
    );
    
    if (new_folder_id == -1) {
        return std::string(CODE_FAIL) + " Failed to create folder (DB Error)\n";
    }
    
    std::cout << "[CmdHandler] Folder created: " << folderName 
              << " (ID=" << new_folder_id << ")" << std::endl;
    
    std::stringstream ss;
    ss << CODE_OK << " Folder created successfully\n";
    ss << "FOLDER_ID:" << new_folder_id << "\n";
    return ss.str();
}

// ======================================================
// LOGIC SHARE FOLDER (ĐỆ QUY / CẤU TRÚC)
// ======================================================

std::string CmdHandler::handleShareFolder(const ClientSession& session, 
                                          long long folder_id, 
                                          const std::string& targetUser) {
    if (!session.isAuthenticated) {
        return std::string(CODE_FAIL) + " Please login first\n";
    }
    
    // Khởi tạo phiên share folder (Copy cấu trúc cây thư mục)
    std::string session_id = FolderShareHandler::getInstance().initiateFolderShare(
        session.username,
        folder_id,
        targetUser
    );
    
    if (session_id.empty()) {
        return std::string(CODE_FAIL) + " Failed to initiate folder share\n";
    }
    
    // Lấy thông tin session vừa tạo
    auto share_session = FolderShareHandler::getInstance().getSession(session_id);
    if (!share_session) {
        return std::string(CODE_FAIL) + " Session creation failed\n";
    }
    
    // Trả về danh sách file cần upload để Client biết
    std::stringstream response;
    response << CODE_OK << " Folder share initiated\n";
    response << "SESSION_ID:" << session_id << "\n";
    response << "TOTAL_FILES:" << share_session->total_files << "\n";
    response << "FILES:\n";
    
    for (const auto& file : share_session->files_to_transfer) {
        response << file.old_file_id << "|" 
                 << file.name << "|" 
                 << file.relative_path << "|" 
                 << file.size_bytes << "\n";
    }
    
    return response.str();
}

std::string CmdHandler::handleGetFolderStructure(const ClientSession& session, 
                                                 long long folder_id) {
    if (!session.isAuthenticated) {
        return std::string(CODE_FAIL) + " Please login first\n";
    }
    
    // Lấy cấu trúc thư mục từ DB
    auto structure = DBManager::getInstance().getFolderStructure(folder_id, session.username);
    
    if (structure.empty()) {
        return "404 Folder not found or empty\n";
    }
    
    // Trả về cấu trúc cây
    std::stringstream response;
    response << CODE_OK << " OK\n";
    response << "FOLDER_ID:" << folder_id << "\n";
    response << "ITEMS:\n";
    
    for (const auto& item : structure) {
        response << item.file_id << "|"
                 << item.name << "|"
                 << (item.is_folder ? "FOLDER" : "FILE") << "|"
                 << item.size << "|"
                 << item.parent_id << "\n";
    }
    
    return response.str();
}