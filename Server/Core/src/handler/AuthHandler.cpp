#include "../../include/request_handler.h"
#include "../../include/db_manager.h"
#include "../../../../Common/Protocol.h"
#include <iostream>

std::string AuthHandler::handleUser(int fd, ClientSession& session, const std::string& username) {
    std::cout << "[AuthHandler::USER] FD: " << fd << ", Username: " << username << std::endl;
    session.username = username;
    return "331 Password required\n";
}

std::string AuthHandler::handlePass(int fd, ClientSession& session, const std::string& password) {
    std::cout << "[SERVER] ===== LOGIN ATTEMPT =====" << std::endl;
    std::cout << "[SERVER] Cmd: PASS from user " << session.username << std::endl;
    std::cout << "[AuthHandler::PASS] FD: " << fd << ", Username: " << session.username << std::endl;
    
    if (session.username.empty()) {
        std::cout << "[AuthHandler::PASS] No username provided" << std::endl;
        return std::string(CODE_FAIL) + " Login with USER first\n";
    }

    if (DBManager::getInstance().checkUser(session.username, password)) {
        session.isAuthenticated = true;
        std::cout << "[SERVER] LOGIN SUCCESS: " << session.username << std::endl;
        std::cout << "[AuthHandler::PASS] Login SUCCESS for " << session.username << std::endl;
        return std::string(CODE_LOGIN_SUCCESS) + " Login successful\n";
    } else {
        std::cerr << "[SERVER] LOGIN FAILED: Invalid credentials for " << session.username << std::endl;
        std::cout << "[AuthHandler::PASS] Login FAILED for " << session.username << std::endl;
        return std::string(CODE_LOGIN_FAIL) + " Login failed\n";
    }
}

std::string AuthHandler::handleRegister(const std::string& username, const std::string& password) {
    std::cout << "[SERVER] ===== REGISTER ATTEMPT =====" << std::endl;
    std::cout << "[SERVER] Cmd: REGISTER " << username << std::endl;
    std::cout << "[AuthHandler::REGISTER] Username: " << username << ", Password length: " << password.length() << std::endl;
    
    if (username.empty() || password.empty()) {
        std::cout << "[AuthHandler::REGISTER] Empty credentials" << std::endl;
        return std::string(CODE_FAIL) + " Username and password cannot be empty\n";
    }
    
    if (username.length() < 3) {
        std::cout << "[AuthHandler::REGISTER] Username too short" << std::endl;
        return std::string(CODE_FAIL) + " Username must be at least 3 characters\n";
    }
    
    if (password.length() < 4) {
        std::cout << "[AuthHandler::REGISTER] Password too short" << std::endl;
        return std::string(CODE_FAIL) + " Password must be at least 4 characters\n";
    }
    
    if (DBManager::getInstance().registerUser(username, password)) {
        std::cout << "[SERVER] REGISTER SUCCESS: " << username << std::endl;
        std::cout << "[AuthHandler::REGISTER] SUCCESS for " << username << std::endl;
        return std::string(CODE_OK) + " Registration successful\n";
    } else {
        std::cerr << "[SERVER] REGISTER FAILED: Username '" << username << "' already exists" << std::endl;
        std::cout << "[AuthHandler::REGISTER] FAILED for " << username << " - username exists" << std::endl;
        return std::string(CODE_FAIL) + " Username already exists\n";
    }
}