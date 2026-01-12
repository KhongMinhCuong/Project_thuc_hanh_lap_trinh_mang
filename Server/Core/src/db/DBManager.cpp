#include "../../include/db_manager.h"
#include "../../include/db_config.h"
#include <iostream>
#include <sstream>
#include <openssl/sha.h>
#include <iomanip>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <cerrno>

// Define STORAGE_PATH if not already defined
#ifndef STORAGE_PATH
#define STORAGE_PATH "./storage/"
#endif

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

    if (!mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASS, 
                            DB_NAME, DB_PORT, nullptr, 0)) {
        std::cerr << "[DB] Connection failed: " << mysql_error(conn) << std::endl;
        return false;
    }

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
    
    return found;
}

bool DBManager::registerUser(std::string username, std::string password) {
    if (!conn) {
        std::cerr << "[DB] Not connected to database" << std::endl;
        return false;
    }

    std::string check_query = "SELECT user_id FROM USERS WHERE username = '" + username + "'";
    if (mysql_query(conn, check_query.c_str())) {
        std::cerr << "[DB] Check query failed: " << mysql_error(conn) << std::endl;
        return false;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) {
        std::cerr << "[DB] Failed to get result: " << mysql_error(conn) << std::endl;
        return false;
    }

    bool exists = (mysql_num_rows(result) > 0);
    mysql_free_result(result);

    if (exists) {
        std::cerr << "[DB] Username '" << username << "' already exists" << std::endl;
        return false;
    }

    std::string hashed_pass = sha256(password);
    
    std::string insert_query = "INSERT INTO USERS (username, password_hash, created_at) VALUES ('" + 
                               username + "', '" + hashed_pass + "', NOW())";
    
    if (mysql_query(conn, insert_query.c_str())) {
        std::cerr << "[DB] Insert failed: " << mysql_error(conn) << std::endl;
        return false;
    }

    return true;
}

std::vector<FileRecord> DBManager::getFiles(std::string username, long long parent_id) {
    std::vector<FileRecord> list;
    if (!conn) return list;

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

    if (parent_id == 0) {
        query = "SELECT f.name, f.size_bytes, u.username, f.created_at, f.file_id, f.is_folder "
                "FROM FILES f "
                "JOIN USERS u ON f.owner_id = u.user_id "
                "WHERE f.file_id != 1 "
                "AND f.owner_id = " + user_id + " "
                "AND f.parent_id IS NULL "
                "AND f.is_deleted = FALSE "
                "ORDER BY f.is_folder DESC, f.created_at DESC";
    } else {
        query = "SELECT f.name, f.size_bytes, u.username, f.created_at, f.file_id, f.is_folder "
                "FROM FILES f "
                "JOIN USERS u ON f.owner_id = u.user_id "
                "WHERE f.file_id != 1 "
                "AND f.owner_id = " + user_id + " "
                "AND f.parent_id = " + std::to_string(parent_id) + " "
                "AND f.is_deleted = FALSE "
                "ORDER BY f.is_folder DESC, f.created_at DESC";
    }

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
        rec.file_id = row[4] ? std::stoll(row[4]) : 0;
        rec.is_folder = row[5] && std::string(row[5]) == "1";
        
        if (rec.is_folder) {
            std::string size_query = 
                "SELECT COALESCE(SUM(size_bytes), 0) FROM FILES "
                "WHERE parent_id = " + std::to_string(rec.file_id) + " "
                "AND is_folder = FALSE AND is_deleted = FALSE";
            
            if (mysql_query(conn, size_query.c_str()) == 0) {
                MYSQL_RES* size_result = mysql_store_result(conn);
                if (size_result) {
                    MYSQL_ROW size_row = mysql_fetch_row(size_result);
                    if (size_row && size_row[0]) {
                        rec.size = std::stol(size_row[0]);
                    }
                    mysql_free_result(size_result);
                }
            }
        }
        
        list.push_back(rec);
    }

    mysql_free_result(result);
    return list;
}

std::vector<FileRecord> DBManager::getSharedFiles(std::string username) {
    std::vector<FileRecord> list;
    if (!conn) return list;

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


    query = "SELECT f.name, f.size_bytes, u.username, f.file_id, f.is_folder "
            "FROM FILES f "
            "JOIN USERS u ON f.owner_id = u.user_id "
            "JOIN SHAREDFILES sf ON f.file_id = sf.file_id "
            "WHERE sf.user_id = " + user_id + " "
            "AND f.owner_id != " + user_id + " "
            "AND f.is_deleted = FALSE "
            "ORDER BY f.is_folder DESC, sf.shared_at DESC";

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
        rec.file_id = row[3] ? std::stoll(row[3]) : 0;
        rec.is_folder = row[4] && std::string(row[4]) == "1";
        
        if (rec.is_folder) {
            std::string size_query = 
                "SELECT COALESCE(SUM(size_bytes), 0) FROM FILES "
                "WHERE parent_id = " + std::to_string(rec.file_id) + " "
                "AND is_folder = FALSE AND is_deleted = FALSE";
            
            if (mysql_query(conn, size_query.c_str()) == 0) {
                MYSQL_RES* size_result = mysql_store_result(conn);
                if (size_result) {
                    MYSQL_ROW size_row = mysql_fetch_row(size_result);
                    if (size_row && size_row[0]) {
                        rec.size = std::stol(size_row[0]);
                    }
                    mysql_free_result(size_result);
                }
            }
        }
        
        list.push_back(rec);
    }

    mysql_free_result(result);
    return list;
}

std::vector<FileRecord> DBManager::getSharedFiles(std::string username, long long parent_id) {
    std::vector<FileRecord> list;
    if (!conn) return list;

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

    if (parent_id > 0) {
        if (!hasSharedAccess(parent_id, username)) {
            std::cerr << "[DB] User '" << username << "' has no access to folder " << parent_id << std::endl;
            return list;
        }
    }

    query = "SELECT f.name, f.size_bytes, u.username, f.file_id, f.is_folder "
            "FROM FILES f "
            "JOIN USERS u ON f.owner_id = u.user_id "
            "WHERE f.parent_id = " + std::to_string(parent_id) + " "
            "AND f.is_deleted = FALSE "
            "ORDER BY f.is_folder DESC, f.created_at DESC";

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
        rec.file_id = row[3] ? std::stoll(row[3]) : 0;
        rec.is_folder = row[4] && std::string(row[4]) == "1";
        
        if (rec.is_folder) {
            std::string size_query = 
                "SELECT COALESCE(SUM(size_bytes), 0) FROM FILES "
                "WHERE parent_id = " + std::to_string(rec.file_id) + " "
                "AND is_folder = FALSE AND is_deleted = FALSE";
            
            if (mysql_query(conn, size_query.c_str()) == 0) {
                MYSQL_RES* size_result = mysql_store_result(conn);
                if (size_result) {
                    MYSQL_ROW size_row = mysql_fetch_row(size_result);
                    if (size_row && size_row[0]) {
                        rec.size = std::stol(size_row[0]);
                    }
                    mysql_free_result(size_result);
                }
            }
        }
        
        list.push_back(rec);
    }

    mysql_free_result(result);
    std::cout << "[DB] Retrieved " << list.size() << " items in shared folder " << parent_id 
              << " for user '" << username << "'" << std::endl;
    return list;
}

bool DBManager::hasSharedAccess(long long file_id, std::string username) {
    if (!conn) return false;

    std::string query = "SELECT user_id FROM USERS WHERE username = '" + username + "'";
    if (mysql_query(conn, query.c_str())) return false;
    
    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) return false;
    
    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row) {
        mysql_free_result(result);
        return false;
    }
    std::string user_id = row[0];
    mysql_free_result(result);

    long long current_id = file_id;
    
    while (current_id > 0) {
        query = "SELECT COUNT(*) FROM SHAREDFILES "
                "WHERE file_id = " + std::to_string(current_id) + " "
                "AND user_id = " + user_id;
        
        if (mysql_query(conn, query.c_str())) return false;
        
        result = mysql_store_result(conn);
        if (!result) return false;
        
        row = mysql_fetch_row(result);
        if (row && row[0] && std::stoi(row[0]) > 0) {
            mysql_free_result(result);
            return true;
        }
        mysql_free_result(result);
        
        query = "SELECT parent_id FROM FILES WHERE file_id = " + std::to_string(current_id);
        if (mysql_query(conn, query.c_str())) return false;
        
        result = mysql_store_result(conn);
        if (!result) return false;
        
        row = mysql_fetch_row(result);
        if (!row || !row[0]) {
            mysql_free_result(result);
            break;
        }
        
        current_id = std::stoll(row[0]);
        mysql_free_result(result);
    }
    
    query = "SELECT COUNT(*) FROM FILES f "
            "JOIN USERS u ON f.owner_id = u.user_id "
            "WHERE f.file_id = " + std::to_string(file_id) + " "
            "AND u.username = '" + username + "'";
    
    if (mysql_query(conn, query.c_str())) return false;
    
    result = mysql_store_result(conn);
    if (!result) return false;
    
    row = mysql_fetch_row(result);
    bool is_owner = (row && row[0] && std::stoi(row[0]) > 0);
    mysql_free_result(result);
    
    return is_owner;
}

bool DBManager::addFile(std::string filename, long filesize, std::string owner, long long parent_id) {
    if (!conn) return false;

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

    std::stringstream ss;
    if (parent_id == 0) {
        ss << "INSERT INTO FILES (owner_id, parent_id, name, is_folder, size_bytes) VALUES ("
           << owner_id << ", NULL, '" << filename << "', FALSE, " << filesize << ") "
           << "ON DUPLICATE KEY UPDATE size_bytes = " << filesize 
           << ", is_deleted = FALSE, updated_at = NOW()";
    } else {
        ss << "INSERT INTO FILES (owner_id, parent_id, name, is_folder, size_bytes) VALUES ("
           << owner_id << ", " << parent_id << ", '" << filename << "', FALSE, " << filesize << ") "
           << "ON DUPLICATE KEY UPDATE size_bytes = " << filesize 
           << ", is_deleted = FALSE, updated_at = NOW()";
    }
    
    if (mysql_query(conn, ss.str().c_str())) {
        std::cerr << "[DB] Insert failed: " << mysql_error(conn) << std::endl;
        return false;
    }

    std::cout << "[DB] File '" << filename << "' saved to database (parent_id: " << parent_id << ")" << std::endl;
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

    std::cout << "[DB] shareFile called: file='" << filename << "' owner='" << ownerUsername << "' target='" << targetUsername << "'" << std::endl;

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

    query = "SELECT file_id FROM FILES WHERE name = '" + filename + "' AND owner_id = " + owner_id + " AND is_deleted = FALSE";
    std::cout << "[DB] Searching for file: query='" << query << "'" << std::endl;
    if (mysql_query(conn, query.c_str())) {
        std::cerr << "[DB] Get file_id failed: " << mysql_error(conn) << std::endl;
        return false;
    }
    
    result = mysql_store_result(conn);
    if (!result) return false;
    
    row = mysql_fetch_row(result);
    if (!row) {
        mysql_free_result(result);
        std::cerr << "[DB] File not found or not owned by user: file='" << filename << "' owner_id=" << owner_id << std::endl;
        return false;
    }
    std::string file_id = row[0];
    std::cout << "[DB] Found file_id: " << file_id << std::endl;
    mysql_free_result(result);

    query = "INSERT INTO SHAREDFILES (file_id, user_id, permission_id) VALUES (" 
            + file_id + ", " + target_user_id + ", 1) "
            "ON DUPLICATE KEY UPDATE shared_at = CURRENT_TIMESTAMP";
    
    std::cout << "[DB] Executing share insert: " << query << std::endl;
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

    if (mysql_affected_rows(conn) == 0) {
        std::cerr << "[DB] File not found or user is not the owner" << std::endl;
        return false;
    }

    std::cout << "[DB] File '" << filename << "' deleted by " << username << std::endl;
    return true;
}

bool DBManager::renameFile(long long fileId, std::string newName, std::string username) {
    if (!conn) return false;

    // First, get the file info (old name, type, owner verification)
    std::string checkQuery = "SELECT f.name, f.is_folder FROM FILES f "
                            "JOIN USERS u ON f.owner_id = u.user_id "
                            "WHERE f.file_id = " + std::to_string(fileId) + " "
                            "AND u.username = '" + username + "' "
                            "AND f.is_deleted = FALSE";
    
    if (mysql_query(conn, checkQuery.c_str())) {
        std::cerr << "[DB] Failed to check item: " << mysql_error(conn) << std::endl;
        return false;
    }
    
    MYSQL_RES* result = mysql_store_result(conn);
    if (!result || mysql_num_rows(result) == 0) {
        if (result) mysql_free_result(result);
        std::cerr << "[DB] Item not found or user is not the owner" << std::endl;
        return false;
    }
    
    MYSQL_ROW row = mysql_fetch_row(result);
    std::string oldName = row[0] ? row[0] : "";
    bool isFolder = (row[1] && std::string(row[1]) == "1");
    mysql_free_result(result);
    
    std::string itemType = isFolder ? "FOLDER" : "FILE";
    std::cout << "[DB RENAME " << itemType << "] File ID: " << fileId << ", Renaming: '" << oldName << "' -> '" << newName << "' by user: " << username << std::endl;

    // Handle physical rename based on type
    if (isFolder) {
        // FOLDER: Check if physical directory exists
        std::string oldPath = std::string(STORAGE_PATH) + oldName;
        std::string newPath = std::string(STORAGE_PATH) + newName;
        
        std::cout << "[DB RENAME FOLDER] Physical path: " << oldPath << " -> " << newPath << std::endl;
        
        struct stat st;
        if (stat(oldPath.c_str(), &st) == 0) {
            // Physical folder exists - rename it
            if (!S_ISDIR(st.st_mode)) {
                std::cerr << "[DB RENAME FOLDER] ERROR: Expected folder but found file!" << std::endl;
                return false;
            }
            
            // Rename physical folder first
            if (std::rename(oldPath.c_str(), newPath.c_str()) != 0) {
                std::cerr << "[DB RENAME FOLDER] Physical rename FAILED: " << strerror(errno) << std::endl;
                return false;
            }
            std::cout << "[DB RENAME FOLDER] Physical rename successful" << std::endl;
        } else {
            // Physical folder doesn't exist - just update database (empty folder from schema)
            std::cout << "[DB RENAME FOLDER] Physical folder not found, updating database only (empty folder)" << std::endl;
        }
    } else {
        // FILE: Only update database, no physical rename needed
        std::cout << "[DB RENAME FILE] Skipping physical rename for file (only updating database)" << std::endl;
    }

    // Update database
    std::string updateQuery = "UPDATE FILES f "
                             "JOIN USERS u ON f.owner_id = u.user_id "
                             "SET f.name = '" + newName + "' "
                             "WHERE f.file_id = " + std::to_string(fileId) + " "
                             "AND u.username = '" + username + "' "
                             "AND f.is_deleted = FALSE";
    
    if (mysql_query(conn, updateQuery.c_str())) {
        std::cerr << "[DB RENAME " << itemType << "] Database update failed: " << mysql_error(conn) << std::endl;
        // Rollback physical rename if it was a folder
        if (isFolder) {
            std::string oldPath = std::string(STORAGE_PATH) + oldName;
            std::string newPath = std::string(STORAGE_PATH) + newName;
            std::rename(newPath.c_str(), oldPath.c_str());
            std::cerr << "[DB RENAME " << itemType << "] Rolled back physical rename" << std::endl;
        }
        return false;
    }

    if (mysql_affected_rows(conn) == 0) {
        std::cerr << "[DB RENAME " << itemType << "] No rows affected" << std::endl;
        // Rollback physical rename if it was a folder
        if (isFolder) {
            std::string oldPath = std::string(STORAGE_PATH) + oldName;
            std::string newPath = std::string(STORAGE_PATH) + newName;
            std::rename(newPath.c_str(), oldPath.c_str());
            std::cerr << "[DB RENAME " << itemType << "] Rolled back physical rename" << std::endl;
        }
        return false;
    }
    
    std::cout << "[DB RENAME " << itemType << "] Database updated successfully" << std::endl;

    std::cout << "[DB RENAME " << itemType << "] SUCCESS: '" << oldName << "' -> '" << newName << "'" << std::endl;
    return true;
}

std::vector<FileRecordEx> DBManager::getItemsInFolder(long long parent_id, std::string username) {
    std::vector<FileRecordEx> list;
    if (!conn) return list;

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

    std::stringstream ss;
    if (parent_id == -1) {
        ss << "SELECT file_id, owner_id, parent_id, name, is_folder, size_bytes, created_at "
           << "FROM FILES WHERE parent_id = 1 AND owner_id = " << user_id 
           << " AND is_deleted = FALSE ORDER BY is_folder DESC, name ASC";
    } else {
        ss << "SELECT file_id, owner_id, parent_id, name, is_folder, size_bytes, created_at "
           << "FROM FILES WHERE parent_id = " << parent_id 
           << " AND is_deleted = FALSE ORDER BY is_folder DESC, name ASC";
    }
    
    if (mysql_query(conn, ss.str().c_str())) {
        std::cerr << "[DB] Query failed: " << mysql_error(conn) << std::endl;
        return list;
    }

    result = mysql_store_result(conn);
    if (!result) return list;

    while ((row = mysql_fetch_row(result))) {
        FileRecordEx rec;
        rec.file_id = row[0] ? std::stoll(row[0]) : 0;
        rec.owner_id = row[1] ? std::stoll(row[1]) : 0;
        rec.parent_id = row[2] ? std::stoll(row[2]) : -1;
        rec.name = row[3] ? row[3] : "";
        rec.is_folder = row[4] && std::string(row[4]) == "1";
        rec.size = row[5] ? std::stoll(row[5]) : 0;
        rec.owner = username;
        list.push_back(rec);
    }

    mysql_free_result(result);
    std::cout << "[DB] Retrieved " << list.size() << " items in folder (parent_id=" 
              << parent_id << ")" << std::endl;
    return list;
}

void DBManager::collectFilesRecursive(long long parent_id, std::vector<FileRecordEx>& results, std::string username) {
    auto items = getItemsInFolder(parent_id, username);
    
    for (const auto& item : items) {
        results.push_back(item);
        
        if (item.is_folder) {
            collectFilesRecursive(item.file_id, results, username);
        }
    }
}

std::vector<FileRecordEx> DBManager::getFolderStructure(long long folder_id, std::string username) {
    std::vector<FileRecordEx> allFiles;
    if (!conn) return allFiles;

    std::string query = "SELECT user_id FROM USERS WHERE username = '" + username + "'";
    if (mysql_query(conn, query.c_str())) return allFiles;
    
    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) return allFiles;
    
    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row) {
        mysql_free_result(result);
        return allFiles;
    }
    std::string user_id = row[0];
    mysql_free_result(result);

    query = "SELECT file_id FROM FILES WHERE file_id = " + std::to_string(folder_id) +
            " AND owner_id = " + user_id + " AND is_folder = TRUE AND is_deleted = FALSE";
    
    if (mysql_query(conn, query.c_str())) return allFiles;
    result = mysql_store_result(conn);
    if (!result || mysql_num_rows(result) == 0) {
        if (result) mysql_free_result(result);
        std::cerr << "[DB] Folder not found or not owned by user" << std::endl;
        return allFiles;
    }
    mysql_free_result(result);

    collectFilesRecursive(folder_id, allFiles, username);
    
    std::cout << "[DB] Collected " << allFiles.size() << " items in folder structure" << std::endl;
    return allFiles;
}

long long DBManager::createFolder(std::string foldername, long long parent_id, std::string owner) {
    if (!conn) return -1;

    std::string query = "SELECT user_id FROM USERS WHERE username = '" + owner + "'";
    if (mysql_query(conn, query.c_str())) return -1;
    
    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) return -1;
    
    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row) {
        mysql_free_result(result);
        return -1;
    }
    std::string owner_id = row[0];
    mysql_free_result(result);

    std::stringstream ss;
    if (parent_id == -1 || parent_id == 0) {
        // Root level folder - parent is NULL or 1
        ss << "INSERT INTO FILES (owner_id, parent_id, name, is_folder, size_bytes) VALUES ("
           << owner_id << ", NULL, '" << foldername << "', TRUE, 0)";
    } else {
        ss << "INSERT INTO FILES (owner_id, parent_id, name, is_folder, size_bytes) VALUES ("
           << owner_id << ", " << parent_id << ", '" << foldername << "', TRUE, 0)";
    }
    
    if (mysql_query(conn, ss.str().c_str())) {
        std::cerr << "[DB] Create folder failed: " << mysql_error(conn) << std::endl;
        return -1;
    }

    long long new_folder_id = mysql_insert_id(conn);
    std::cout << "[DB] Folder '" << foldername << "' created with ID: " << new_folder_id << std::endl;
    return new_folder_id;
}

long long DBManager::createFileInFolder(std::string filename, long long parent_id, long long filesize, std::string owner) {
    if (!conn) return -1;

    std::string query = "SELECT user_id FROM USERS WHERE username = '" + owner + "'";
    if (mysql_query(conn, query.c_str())) return -1;
    
    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) return -1;
    
    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row) {
        mysql_free_result(result);
        return -1;
    }
    std::string owner_id = row[0];
    mysql_free_result(result);

    std::stringstream ss;
    ss << "INSERT INTO FILES (owner_id, parent_id, name, is_folder, size_bytes) VALUES ("
       << owner_id << ", " << parent_id << ", '" << filename << "', FALSE, " << filesize << ")";
    
    if (mysql_query(conn, ss.str().c_str())) {
        std::cerr << "[DB] Create file failed: " << mysql_error(conn) << std::endl;
        return -1;
    }

    long long new_file_id = mysql_insert_id(conn);
    std::cout << "[DB] File '" << filename << "' created in folder " << parent_id 
              << " with ID: " << new_file_id << std::endl;
    return new_file_id;
}

FileRecordEx DBManager::getFileInfo(long long file_id) {
    FileRecordEx rec;
    rec.file_id = -1;
    if (!conn) return rec;

    std::string query = "SELECT f.file_id, f.owner_id, f.parent_id, f.name, f.is_folder, "
                       "f.size_bytes, u.username FROM FILES f "
                       "JOIN USERS u ON f.owner_id = u.user_id "
                       "WHERE f.file_id = " + std::to_string(file_id);
    
    if (mysql_query(conn, query.c_str())) {
        std::cerr << "[DB] Query failed: " << mysql_error(conn) << std::endl;
        return rec;
    }

    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) return rec;
    
    MYSQL_ROW row = mysql_fetch_row(result);
    if (row) {
        rec.file_id = std::stoll(row[0]);
        rec.owner_id = std::stoll(row[1]);
        rec.parent_id = row[2] ? std::stoll(row[2]) : -1;
        rec.name = row[3] ? row[3] : "";
        rec.is_folder = row[4] && std::string(row[4]) == "1";
        rec.size = row[5] ? std::stoll(row[5]) : 0;
        rec.owner = row[6] ? row[6] : "";
    }
    
    mysql_free_result(result);
    return rec;
}

bool DBManager::shareFolderWithUser(long long folder_id, std::string targetUsername) {
    if (!conn) return false;

    std::cout << "[DB] shareFolderWithUser called: folder_id=" << folder_id << " target='" << targetUsername << "'" << std::endl;

    std::string query = "SELECT user_id FROM USERS WHERE username = '" + targetUsername + "'";
    if (mysql_query(conn, query.c_str())) {
        std::cerr << "[DB] Get target_user_id failed: " << mysql_error(conn) << std::endl;
        return false;
    }
    
    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) return false;
    
    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row) {
        mysql_free_result(result);
        std::cerr << "[DB] Target user not found: " << targetUsername << std::endl;
        return false;
    }
    std::string target_user_id = row[0];
    mysql_free_result(result);
    std::cout << "[DB] Found target user_id: " << target_user_id << std::endl;

    query = "INSERT INTO SHAREDFILES (file_id, user_id, permission_id) VALUES (" 
            + std::to_string(folder_id) + ", " + target_user_id + ", 1) "
            "ON DUPLICATE KEY UPDATE shared_at = CURRENT_TIMESTAMP";
    
    std::cout << "[DB] Executing folder share: " << query << std::endl;
    if (mysql_query(conn, query.c_str())) {
        std::cerr << "[DB] Share folder failed: " << mysql_error(conn) << std::endl;
        return false;
    }

    std::cout << "[DB] Folder (ID=" << folder_id << ") shared with " << targetUsername 
              << " (recursive access via hasSharedAccess)" << std::endl;
    return true;
}

bool DBManager::isFileSharedWithUser(std::string filename, std::string username) {
    if (!conn) return false;
    
    std::string query = "SELECT file_id FROM FILES WHERE name = '" + filename + "' AND is_deleted = FALSE";
    if (mysql_query(conn, query.c_str())) {
        std::cerr << "[DB] isFileSharedWithUser query failed: " << mysql_error(conn) << std::endl;
        return false;
    }
    
    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) return false;
    
    MYSQL_ROW row = mysql_fetch_row(result);
    if (!row) {
        mysql_free_result(result);
        return false;
    }
    long long file_id = std::stoll(row[0]);
    mysql_free_result(result);
    
    query = "SELECT user_id FROM USERS WHERE username = '" + username + "'";
    if (mysql_query(conn, query.c_str())) {
        return false;
    }
    
    result = mysql_store_result(conn);
    if (!result) return false;
    
    row = mysql_fetch_row(result);
    if (!row) {
        mysql_free_result(result);
        return false;
    }
    std::string user_id = row[0];
    mysql_free_result(result);
    
    query = "SELECT COUNT(*) FROM SHAREDFILES WHERE file_id = " + std::to_string(file_id) 
            + " AND user_id = " + user_id;
    
    if (mysql_query(conn, query.c_str())) {
        return false;
    }
    
    result = mysql_store_result(conn);
    if (!result) return false;
    
    row = mysql_fetch_row(result);
    bool isShared = (row && std::stoi(row[0]) > 0);
    mysql_free_result(result);
    
    return isShared;
}

std::string DBManager::getFileOwner(std::string filename) {
    if (!conn) return "";
    
    std::string query = "SELECT u.username FROM FILES f "
                       "JOIN USERS u ON f.owner_id = u.user_id "
                       "WHERE f.name = '" + filename + "' AND f.is_deleted = FALSE";
    
    if (mysql_query(conn, query.c_str())) {
        std::cerr << "[DB] getFileOwner query failed: " << mysql_error(conn) << std::endl;
        return "";
    }
    
    MYSQL_RES* result = mysql_store_result(conn);
    if (!result) return "";
    
    MYSQL_ROW row = mysql_fetch_row(result);
    std::string owner = "";
    if (row && row[0]) {
        owner = row[0];
    }
    mysql_free_result(result);
    
    return owner;
}
