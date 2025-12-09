#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include <string>
#include <vector>
#include "server.h" // Chứa struct ClientSession

// Xử lý xác thực (Login, Register)
class AuthHandler {
public:
    static std::string handleUser(int fd, ClientSession& session, const std::string& username);
    static std::string handlePass(int fd, ClientSession& session, const std::string& password);
    static std::string handleRegister(const std::string& username, const std::string& password);
};

// Xử lý lệnh quản lý file (List, Search, Share)
class CmdHandler {
public:
    static std::string handleList(const ClientSession& session);
    static std::string handleListShared(const ClientSession& session);
    static std::string handleSearch(const ClientSession& session, const std::string& keyword);
    static std::string handleShare(const ClientSession& session, const std::string& filename, const std::string& targetUser);
    static std::string handleDelete(const ClientSession& session, const std::string& filename);
};

// Xử lý chuẩn bị I/O (Quota check, Permission check trước khi upload/download)
class FileIOHandler {
public:
    static std::string handleQuotaCheck(const ClientSession& session, long filesize);
    static bool checkDownloadPermission(const ClientSession& session, const std::string& filename);
};

#endif // REQUEST_HANDLER_H