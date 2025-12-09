#include "../../include/db_manager.h"
#include <iostream>
#include <sstream>
#include <openssl/sha.h>
#include <iomanip>

// Hàm hash SHA256
std::string sha256(const std::string& str) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)str.c_str(), str.size(), hash);
    
    std::stringstream ss;
    for(int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

bool DBManager::connect() {
    conn = mysql_init(nullptr);
    if (!conn) {
        std::cerr << "[DB] mysql_init() failed" << std::endl;
        return false;
    }

    if (!mysql_real_connect(conn, "localhost", "cuong", "040424", 
                            "file_management", 0, nullptr, 0)) {
        std::cerr << "[DB] Connection failed: " << mysql_error(conn) << std::endl;
        return false;
    }

    std::cout << "[DB] Connected to MySQL database 'file_management'" << std::endl;
    return true;
}

void DBManager::disconnect() {
    if (conn) {
        mysql_close(conn);
        conn = nullptr;
    }
}

bool DBManager::checkUser(std::string user, std::string pass) {
    if (!conn) return false;

    std::string hashed_pass = sha256(pass);
    
    std::string query = "SELECT user_id FROM USERS WHERE username = '" + user + 
                       "' AND password_hash = '" + hashed_pass + "'";
    
    if (mysql_query(conn, query.c_str())) {
        std::cerr << "[DB] Query failed: " << mysql_error(conn) << std::endl;
        return false;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) return false;

    bool found = (mysql_num_rows(result) > 0);
    mysql_free_result(result);
    
    std::cout << "[DB] User '" << user << "' authentication: " 
              << (found ? "SUCCESS" : "FAILED") << std::endl;
    return found;
}

std::vector<FileRecord> DBManager::getFiles(std::string username) {
    std::vector<FileRecord> list;
    if (!conn) return list;

    // Lấy user_id
    std::string query = "SELECT user_id FROM USERS WHERE username = '" + username + "'";
    if (mysql_query(conn, query.c_str())) return list;
    
    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) return list;
    
    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row) {
        mysql_free_result(result);
        return list;
    }
    std::string user_id = row[0];
    mysql_free_result(result);

    // Lấy CHỈ files do user sở hữu (không bao gồm shared)
    query = "SELECT f.name, f.size_bytes, u.username, f.created_at "
            "FROM FILES f "
            "JOIN USERS u ON f.owner_id = u.user_id "
            "WHERE f.file_id != 1 "
            "AND f.owner_id = " + user_id + " "
            "AND f.is_deleted = FALSE AND f.is_folder = FALSE "
            "ORDER BY f.created_at DESC";

    if (mysql_query(conn, query.c_str())) {
        std::cerr << "[DB] Query failed: " << mysql_error(conn) << std::endl;
        return list;
    }

    result = mysql_store_result(conn);
    if (!result) return list;

    while ((row = mysql_fetch_row(result))) {
        FileRecord rec;
        rec.name = row[0] ? row[0] : "";
        rec.size = row[1] ? std::stol(row[1]) : 0;
        rec.owner = row[2] ? row[2] : "";
        list.push_back(rec);
    }

    mysql_free_result(result);
    std::cout << "[DB] Retrieved " << list.size() << " files for user '" << username << "'" << std::endl;
    return list;
}

std::vector<FileRecord> DBManager::getSharedFiles(std::string username) {
    std::vector<FileRecord> list;
    if (!conn) return list;

    // Lấy user_id
    std::string query = "SELECT user_id FROM USERS WHERE username = '" + username + "'";
    if (mysql_query(conn, query.c_str())) return list;
    
    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) return list;
    
    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row) {
        mysql_free_result(result);
        return list;
    }
    std::string user_id = row[0];
    mysql_free_result(result);

    // Lấy CHỈ file được share (không bao gồm file của mình)
    query = "SELECT f.name, f.size_bytes, u.username "
            "FROM FILES f "
            "JOIN USERS u ON f.owner_id = u.user_id "
            "JOIN SHAREDFILES sf ON f.file_id = sf.file_id "
            "WHERE sf.user_id = " + user_id + " "
            "AND f.owner_id != " + user_id + " "
            "AND f.is_deleted = FALSE AND f.is_folder = FALSE "
            "ORDER BY sf.shared_at DESC";

    if (mysql_query(conn, query.c_str())) {
        std::cerr << "[DB] Query failed: " << mysql_error(conn) << std::endl;
        return list;
    }

    result = mysql_store_result(conn);
    if (!result) return list;

    while ((row = mysql_fetch_row(result))) {
        FileRecord rec;
        rec.name = row[0] ? row[0] : "";
        rec.size = row[1] ? std::stol(row[1]) : 0;
        rec.owner = row[2] ? row[2] : "";
        list.push_back(rec);
    }

    mysql_free_result(result);
    std::cout << "[DB] Retrieved " << list.size() << " shared files for user '" << username << "'" << std::endl;
    return list;
}

bool DBManager::addFile(std::string filename, long filesize, std::string owner) {
    if (!conn) return false;

    // Lấy owner_id
    std::string query = "SELECT user_id FROM USERS WHERE username = '" + owner + "'";
    if (mysql_query(conn, query.c_str())) return false;
    
    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) return false;
    
    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row) {
        mysql_free_result(result);
        return false;
    }
    std::string owner_id = row[0];
    mysql_free_result(result);

    // Insert file vào thư mục root (parent_id=1)
    std::stringstream ss;
    ss << "INSERT INTO FILES (owner_id, parent_id, name, is_folder, size_bytes) VALUES ("
       << owner_id << ", 1, '" << filename << "', FALSE, " << filesize << ") "
       << "ON DUPLICATE KEY UPDATE size_bytes = " << filesize 
       << ", is_deleted = FALSE, updated_at = NOW()";
    
    if (mysql_query(conn, ss.str().c_str())) {
        std::cerr << "[DB] Insert failed: " << mysql_error(conn) << std::endl;
        return false;
    }

    std::cout << "[DB] File '" << filename << "' saved to database" << std::endl;
    return true;
}

long DBManager::getStorageUsed(std::string username) {
    if (!conn) return 0;

    std::string query = "SELECT COALESCE(SUM(f.size_bytes), 0) FROM FILES f "
                       "JOIN USERS u ON f.owner_id = u.user_id "
                       "WHERE u.username = '" + username + "' AND f.is_deleted = FALSE";
    
    if (mysql_query(conn, query.c_str())) return 0;
    
    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) return 0;
    
    MYSQL_ROW row = mysql_fetch_row(result);
    long used = row && row[0] ? std::stol(row[0]) : 0;
    
    mysql_free_result(result);
    return used;
}

bool DBManager::shareFile(std::string filename, std::string ownerUsername, std::string targetUsername) {
    if (!conn) return false;

    // 1. Lấy owner_id
    std::string query = "SELECT user_id FROM USERS WHERE username = '" + ownerUsername + "'";
    if (mysql_query(conn, query.c_str())) {
        std::cerr << "[DB] Get owner_id failed: " << mysql_error(conn) << std::endl;
        return false;
    }
    
    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) return false;
    
    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row) {
        mysql_free_result(result);
        return false;
    }
    std::string owner_id = row[0];
    mysql_free_result(result);

    // 2. Lấy target_user_id
    query = "SELECT user_id FROM USERS WHERE username = '" + targetUsername + "'";
    if (mysql_query(conn, query.c_str())) {
        std::cerr << "[DB] Get target_user_id failed: " << mysql_error(conn) << std::endl;
        return false;
    }
    
    result = mysql_store_result(conn);
    if (!result) return false;
    
    row = mysql_fetch_row(result);
    if (!row) {
        mysql_free_result(result);
        std::cerr << "[DB] Target user not found: " << targetUsername << std::endl;
        return false;
    }
    std::string target_user_id = row[0];
    mysql_free_result(result);

    // 3. Lấy file_id
    query = "SELECT file_id FROM FILES WHERE name = '" + filename + "' AND owner_id = " + owner_id + " AND is_deleted = FALSE";
    if (mysql_query(conn, query.c_str())) {
        std::cerr << "[DB] Get file_id failed: " << mysql_error(conn) << std::endl;
        return false;
    }
    
    result = mysql_store_result(conn);
    if (!result) return false;
    
    row = mysql_fetch_row(result);
    if (!row) {
        mysql_free_result(result);
        std::cerr << "[DB] File not found or not owned by user" << std::endl;
        return false;
    }
    std::string file_id = row[0];
    mysql_free_result(result);

    // 4. Insert vào SHAREDFILES (permission_id = 1 cho READ)
    query = "INSERT INTO SHAREDFILES (file_id, user_id, permission_id) VALUES (" 
            + file_id + ", " + target_user_id + ", 1) "
            "ON DUPLICATE KEY UPDATE shared_at = CURRENT_TIMESTAMP";
    
    if (mysql_query(conn, query.c_str())) {
        std::cerr << "[DB] Share file failed: " << mysql_error(conn) << std::endl;
        return false;
    }

    std::cout << "[DB] File '" << filename << "' shared from " << ownerUsername 
              << " to " << targetUsername << std::endl;
    return true;
}

bool DBManager::deleteFile(std::string filename, std::string username) {
    if (!conn) return false;

    // Soft delete: set is_deleted = TRUE
    // Chỉ owner mới có quyền xóa
    std::string query = "UPDATE FILES f "
                       "JOIN USERS u ON f.owner_id = u.user_id "
                       "SET f.is_deleted = TRUE "
                       "WHERE f.name = '" + filename + "' "
                       "AND u.username = '" + username + "' "
                       "AND f.is_deleted = FALSE";
    
    if (mysql_query(conn, query.c_str())) {
        std::cerr << "[DB] Delete file failed: " << mysql_error(conn) << std::endl;
        return false;
    }

    // Kiểm tra xem có file nào bị ảnh hưởng không
    if (mysql_affected_rows(conn) == 0) {
        std::cerr << "[DB] File not found or user is not the owner" << std::endl;
        return false;
    }

    std::cout << "[DB] File '" << filename << "' deleted by " << username << std::endl;
    return true;
}