#include "network_manager.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QTimer>
#include <QThread>

NetworkManager::NetworkManager(QObject *parent) : QObject(parent) {
    socket = new QTcpSocket(this);
    
    // Kết nối signal đọc dữ liệu cho các lệnh thông thường
    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
}

void NetworkManager::connectToServer(const QString &host, quint16 port) {
    currentHost = host;
    currentPort = port;
    
    socket->connectToHost(host, port);
    if(socket->waitForConnected(3000)) {
        emit connectionStatus(true, "Connected to Server!");
    } else {
        emit connectionStatus(false, "Connection Failed!");
    }
}

// ========================================
// THAY THẾ HÀM login() TRONG network_manager.cpp
// ========================================

void NetworkManager::login(const QString &user, const QString &pass) {
    if(socket->state() != QAbstractSocket::ConnectedState) {
        qDebug() << "[Network] Not connected to server!";
        emit loginFailed("Not connected to server!");
        return;
    }

    qDebug() << "[Network] Starting login process for user:" << user;
    
    // Disconnect readyRead để xử lý đồng bộ
    disconnect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    // 1. Gửi USER command
    QString cmdUser = QString("%1 %2\n").arg(CMD_USER, user);
    socket->write(cmdUser.toUtf8());
    socket->flush();
    
    qDebug() << "[Network] Sent USER command";
    
    // 2. Đợi response "331 Password required"
    if (!socket->waitForReadyRead(3000)) {
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        qDebug() << "[Network] Timeout waiting for USER response";
        emit loginFailed("Timeout: Server not responding");
        return;
    }
    
    QString userResponse = QString::fromUtf8(socket->readAll()).trimmed();
    qDebug() << "[Network] USER response:" << userResponse;
    
    // Check if server is ready for password
    if (!userResponse.startsWith("331")) {
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        qDebug() << "[Network] Unexpected USER response:" << userResponse;
        emit loginFailed("Unexpected server response: " + userResponse);
        return;
    }
    
    // 3. Gửi PASS command (SAU KHI NHẬN 331)
    QString cmdPass = QString("%1 %2\n").arg(CMD_PASS, pass);
    socket->write(cmdPass.toUtf8());
    socket->flush();
    
    qDebug() << "[Network] Sent PASS command";
    
    // 4. Đợi response "230 Login successful" hoặc "530 Login failed"
    if (!socket->waitForReadyRead(3000)) {
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        qDebug() << "[Network] Timeout waiting for PASS response";
        emit loginFailed("Timeout: Server not responding to password");
        return;
    }
    
    QString passResponse = QString::fromUtf8(socket->readAll()).trimmed();
    qDebug() << "[Network] PASS response:" << passResponse;
    
    // Kết nối lại signal
    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    // 5. Kiểm tra kết quả
    if (passResponse.startsWith(CODE_LOGIN_SUCCESS)) {
        qDebug() << "[Network] Login successful!";
        currentUsername = user;
        currentPassword = pass;
        emit loginSuccess();
    } else if (passResponse.startsWith(CODE_LOGIN_FAIL)) {
        qDebug() << "[Network] Login failed: Invalid credentials";
        emit loginFailed("Invalid username or password");
    } else {
        qDebug() << "[Network] Unexpected PASS response:" << passResponse;
        emit loginFailed("Unexpected server response: " + passResponse);
    }
}

void NetworkManager::requestFileList() {
    qDebug() << "[Network] Requesting file list...";
    QString cmd = QString("%1\n").arg(CMD_LIST);
    socket->write(cmd.toUtf8());
    socket->flush();
}

void NetworkManager::requestSharedFileList() {
    qDebug() << "[Network] Requesting shared file list...";
    QString cmd = QString("%1\n").arg(CMD_LISTSHARED);
    socket->write(cmd.toUtf8());
    socket->flush();
}

void NetworkManager::uploadFile(const QString &filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit uploadProgress("Failed to open file!");
        return;
    }

    QFileInfo fileInfo(filePath);
    QString filename = fileInfo.fileName();
    qint64 filesize = fileInfo.size();

    qDebug() << "[Upload] Starting upload:" << filename << "Size:" << filesize;

    // 1. Tạm thời ngắt kết nối signal readyRead để xử lý đồng bộ
    disconnect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);

    // Lambda helper để xử lý lỗi gọn gàng: đóng file, reconnect signal, báo lỗi
    auto handleError = [&](const QString &msg) {
        file.close();
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        emit uploadProgress(msg);
    };

    // 2. Kiểm tra Quota (CMD_UPLOAD_CHECK)
    QString checkCmd = QString("%1 %2 %3\n").arg(CMD_UPLOAD_CHECK, filename).arg(filesize);
    socket->write(checkCmd.toUtf8());
    socket->flush();
    
    if (!socket->waitForReadyRead(5000)) {
        handleError("Timeout: Server not responding to quota check.");
        return;
    }
    
    QString response = QString::fromUtf8(socket->readAll()).trimmed();
    if (!response.startsWith(CODE_OK)) {
        handleError("Upload rejected: " + response);
        return;
    }

    // 3. Gửi lệnh bắt đầu upload (STOR)
    QString storCmd = QString("%1 %2 %3\n").arg(CMD_UPLOAD, filename).arg(filesize);
    socket->write(storCmd.toUtf8());
    socket->flush();
    
    if (!socket->waitForReadyRead(5000)) {
        handleError("Timeout: Server not responding to upload request.");
        return;
    }
    
    response = QString::fromUtf8(socket->readAll()).trimmed();
    if (!response.startsWith(CODE_DATA_OPEN)) {
        handleError("Server rejected data connection: " + response);
        return;
    }

    // 4. Bắt đầu gửi dữ liệu (Chunking)
    qDebug() << "[Upload] Transmitting data...";
    qint64 totalSent = 0;
    char buffer[65536]; // Buffer 64KB (Tối ưu hơn 4KB)

    emit transferProgress(0, filesize); // Reset progress bar

    while (!file.atEnd()) {
        if (socket->state() != QAbstractSocket::ConnectedState) {
            handleError("Network disconnected during upload!");
            return;
        }

        qint64 bytesRead = file.read(buffer, sizeof(buffer));
        if (bytesRead == -1) {
            handleError("Error reading local file.");
            return;
        }

        qint64 bytesWritten = socket->write(buffer, bytesRead);
        if (bytesWritten == -1) {
             handleError("Socket write error.");
             return;
        }
        
        // Đợi dữ liệu được đẩy xuống card mạng để tránh tràn buffer RAM
        socket->waitForBytesWritten(100); 

        totalSent += bytesWritten;
        
        // --- CẬP NHẬT TIẾN ĐỘ ---
        emit transferProgress(totalSent, filesize);
    }
    
    socket->flush();
    file.close();
    
    qDebug() << "[Upload] Data sent, waiting for confirmation...";

    // 5. Đợi xác nhận cuối cùng (226 Transfer Complete)
    // Tăng timeout lên 15s vì server có thể mất thời gian verify file hoặc ghi đĩa
    if (!socket->waitForReadyRead(15000)) {
        handleError("Timeout waiting for server confirmation.");
        return;
    }
    
    response = QString::fromUtf8(socket->readAll()).trimmed();
    
    // Kết nối lại signal bình thường
    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    if (response.startsWith(CODE_TRANSFER_COMPLETE)) {
        emit uploadProgress("Upload successful: " + filename);
        // Refresh danh sách sau 200ms
        QTimer::singleShot(200, this, [this]() {
            requestFileList();
        });
    } else {
        emit uploadProgress("Upload finished but server reported error: " + response);
    }
}

void NetworkManager::downloadFile(const QString &filename, const QString &savePath) {
    if(socket->state() != QAbstractSocket::ConnectedState) {
        emit downloadComplete("Not connected to server!");
        return;
    }

    qDebug() << "[Download] Downloading" << filename << "to" << savePath;
    
    disconnect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    // Helper xử lý lỗi download
    auto handleDownloadError = [&](const QString &msg, QFile &f) {
        f.close();
        f.remove(); // Xóa file tải dở dang
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        emit downloadComplete(msg);
    };

    // 1. Gửi lệnh RETR
    QString cmd = QString("%1 %2\n").arg(CMD_DOWNLOAD, filename);
    socket->write(cmd.toUtf8());
    socket->flush();
    
    if (!socket->waitForReadyRead(5000)) {
        // Cần truyền dummy file vì handleDownloadError yêu cầu tham số QFile
        QFile dummy; 
        // Lưu ý: dummy chưa open nên remove() ko ảnh hưởng gì
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        emit downloadComplete("Timeout: Server not responding to download request.");
        return;
    }

    // Đọc dòng đầu tiên chứa thông tin (VD: "150 102450")
    QString response = QString::fromUtf8(socket->readLine()).trimmed();
    
    if (response.startsWith(CODE_FAIL)) {
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        emit downloadComplete("Server Error: " + response);
        return;
    }

    // Parse filesize
    QStringList parts = response.split(' ');
    qint64 filesize = 0;
    if (parts.size() >= 2) {
        filesize = parts[1].toLongLong();
    }
    
    // 2. Mở file để ghi
    QFile file(savePath);
    if (!file.open(QIODevice::WriteOnly)) {
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        emit downloadComplete("Cannot save file (Permission denied?): " + savePath);
        return;
    }

    // 3. Vòng lặp nhận dữ liệu
    qDebug() << "[Download] Receiving data, size:" << filesize;
    emit transferProgress(0, filesize); // Reset thanh loading

    qint64 totalReceived = 0;
    int retryCount = 0;

    while (totalReceived < filesize) {
        // Chờ dữ liệu đến
        if (!socket->waitForReadyRead(5000)) {
            if (socket->state() != QAbstractSocket::ConnectedState) {
                handleDownloadError("Network connection lost!", file);
                return;
            }
            retryCount++;
            if (retryCount > 3) { // 15s không có dữ liệu -> Timeout
                 handleDownloadError("Data transfer timeout.", file);
                 return;
            }
            continue;
        }
        
        retryCount = 0; // Reset retry nếu nhận được dữ liệu

        QByteArray chunk = socket->readAll();
        qint64 bytesWritten = file.write(chunk);
        
        if (bytesWritten == -1) {
            handleDownloadError("Disk full or write error!", file);
            return;
        }

        totalReceived += bytesWritten;
        
        // --- CẬP NHẬT TIẾN ĐỘ ---
        emit transferProgress(totalReceived, filesize);
    }
    
    file.close();
    qDebug() << "[Download] Transfer finished. Bytes:" << totalReceived;

    // 4. Kiểm tra thông báo hoàn tất (226)
    // Đôi khi server gửi 226 ngay sau data, socket có thể vẫn còn buffer
    if (socket->bytesAvailable() > 0 || socket->waitForReadyRead(2000)) {
        QString finalMsg = QString::fromUtf8(socket->readLine()).trimmed();
        qDebug() << "[Download] Final Message:" << finalMsg;
    }
    
    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    if (totalReceived == filesize) {
        emit downloadComplete("File saved successfully to: " + savePath);
    } else {
        emit downloadComplete("Warning: File size mismatch. Downloaded: " + QString::number(totalReceived));
    }
}

void NetworkManager::shareFile(const QString &filename, const QString &targetUser) {
    if(socket->state() != QAbstractSocket::ConnectedState) {
        emit shareResult(false, "Not connected!");
        return;
    }

    QString cmd = QString("SHARE %1 %2\n").arg(filename, targetUser);
    socket->write(cmd.toUtf8());
    socket->flush();
    
    disconnect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    if (!socket->waitForReadyRead(5000)) {
        emit shareResult(false, "Timeout waiting for share response.");
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        return;
    }
    
    QString response = QString::fromUtf8(socket->readAll()).trimmed();
    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    if (response.startsWith(CODE_OK)) {
        emit shareResult(true, "File shared successfully!");
    } else {
        emit shareResult(false, "Share failed: " + response);
    }
}

void NetworkManager::deleteFile(const QString &filename) {
    if(socket->state() != QAbstractSocket::ConnectedState) {
        emit deleteResult(false, "Not connected!");
        return;
    }

    QString cmd = QString("DELETE %1\n").arg(filename);
    socket->write(cmd.toUtf8());
    socket->flush();
    
    disconnect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    if (!socket->waitForReadyRead(5000)) {
        emit deleteResult(false, "Timeout waiting for delete response.");
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        return;
    }
    
    QString response = QString::fromUtf8(socket->readAll()).trimmed();
    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    if (response.startsWith(CODE_OK)) {
        emit deleteResult(true, "File deleted successfully!");
    } else {
        emit deleteResult(false, "Delete failed: " + response);
    }
}

void NetworkManager::logout() {
    if (socket->state() == QAbstractSocket::ConnectedState) {
        socket->disconnectFromHost();
        // Không chặn luồng bằng waitForDisconnected nếu đang ở luồng chính UI,
        // để Qt tự xử lý sự kiện disconnected.
    }
    currentUsername.clear();
    currentPassword.clear();
    emit logoutSuccess();
}

// ========================================
// THAY THẾ HÀM onReadyRead() TRONG network_manager.cpp
// ========================================

void NetworkManager::onReadyRead() {
    // Đọc TẤT CẢ data available cùng lúc
    QByteArray allData = socket->readAll();
    QString fullResponse = QString::fromUtf8(allData).trimmed();
    
    if (fullResponse.isEmpty()) return;
    
    qDebug() << "========== NETWORK DEBUG ==========";
    qDebug() << "[Network] Raw bytes:" << allData.size();
    qDebug() << "[Network] Full response:\n" << fullResponse;
    qDebug() << "===================================";
    
    // Split thành các dòng
    QStringList lines = fullResponse.split('\n', Qt::SkipEmptyParts);
    qDebug() << "[Network] Total lines:" << lines.size();
    
    // Phân loại responses
    QStringList fileListLines;
    bool hasLoginSuccess = false;
    bool hasLoginFailed = false;
    
    for (const QString &line : lines) {
        QString trimmed = line.trimmed();
        
        qDebug() << "[Network] Processing line:" << trimmed;
        
        // Check login responses
        if (trimmed.startsWith(CODE_LOGIN_SUCCESS)) {
            hasLoginSuccess = true;
            qDebug() << "[Network] Login SUCCESS detected";
        } 
        else if (trimmed.startsWith(CODE_LOGIN_FAIL)) {
            hasLoginFailed = true;
            qDebug() << "[Network] Login FAILED detected";
        }
        // Check for empty list response
        else if (trimmed.startsWith("210")) {
            qDebug() << "[Network] Empty list detected";
            emit fileListReceived(""); // Empty list
            return;
        }
        // Detect file list lines (contains |)
        else if (trimmed.contains('|')) {
            qDebug() << "[Network] File list line detected:" << trimmed;
            fileListLines.append(trimmed);
        }
    }
    
    // Emit login results
    if (hasLoginSuccess) {
        emit loginSuccess();
    }
    if (hasLoginFailed) {
        emit loginFailed("Invalid Username or Password");
    }
    
    // Emit file list nếu có
    if (!fileListLines.isEmpty()) {
        QString fileListData = fileListLines.join("\n");
        qDebug() << "========== FILE LIST DATA ==========";
        qDebug() << "[Network] Emitting" << fileListLines.size() << "files";
        qDebug() << "[Network] File list data:\n" << fileListData;
        qDebug() << "====================================";
        
        emit fileListReceived(fileListData);
    }
}
void NetworkManager::getFolderStructure(long long folder_id) {
    if (socket->state() != QAbstractSocket::ConnectedState) {
        emit folderShareFailed("Not connected to server!");
        return;
    }
    
    qDebug() << "[Network] Getting folder structure for folder_id:" << folder_id;
    
    // Disconnect readyRead để xử lý đồng bộ
    disconnect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    // Gửi command
    QString cmd = QString("%1 %2\n").arg(CMD_GET_FOLDER_STRUCTURE).arg(folder_id);
    socket->write(cmd.toUtf8());
    socket->flush();
    
    // Đợi response
    if (!socket->waitForReadyRead(5000)) {
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        emit folderShareFailed("Timeout getting folder structure");
        return;
    }
    
    // Đọc response
    QString response;
    while (socket->canReadLine()) {
        response += QString::fromUtf8(socket->readLine());
    }
    
    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    qDebug() << "[Network] Folder structure response:" << response;
    
    // Parse response
    QStringList lines = response.split('\n', Qt::SkipEmptyParts);
    
    if (lines.isEmpty() || !lines[0].startsWith("200")) {
        emit folderShareFailed("Failed to get folder structure: " + response);
        return;
    }
    
    // Parse items
    // Format: file_id|name|FOLDER/FILE|size|parent_id
    QList<FileNodeInfo> structure;
    bool inItems = false;
    
    for (const QString &line : lines) {
        if (line.startsWith("ITEMS:")) {
            inItems = true;
            continue;
        }
        
        if (inItems && line.contains('|')) {
            QStringList parts = line.split('|');
            if (parts.size() >= 5) {
                FileNodeInfo info;
                info.file_id = parts[0].toLongLong();
                info.name = parts[1];
                info.is_folder = (parts[2] == "FOLDER");
                info.size = parts[3].toLongLong();
                info.parent_id = parts[4].toLongLong();
                structure.append(info);
            }
        }
    }
    
    qDebug() << "[Network] Parsed" << structure.size() << "items from folder structure";
    emit folderStructureReceived(folder_id, structure);
}

void NetworkManager::shareFolderRequest(long long folder_id, const QString &targetUser) {
    if (socket->state() != QAbstractSocket::ConnectedState) {
        emit folderShareFailed("Not connected to server!");
        return;
    }
    
    qDebug() << "[Network] Initiating folder share: folder_id=" << folder_id 
             << ", target=" << targetUser;
    
    // Disconnect readyRead
    disconnect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    // Gửi command
    QString cmd = QString("%1 %2 %3\n").arg(CMD_SHARE_FOLDER).arg(folder_id).arg(targetUser);
    socket->write(cmd.toUtf8());
    socket->flush();
    
    // Đợi response
    if (!socket->waitForReadyRead(10000)) {
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        emit folderShareFailed("Timeout initiating folder share");
        return;
    }
    
    // Đọc response
    QString response;
    while (socket->canReadLine()) {
        response += QString::fromUtf8(socket->readLine());
    }
    
    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    qDebug() << "[Network] Share folder response:" << response;
    
    // Parse response
    QStringList lines = response.split('\n', Qt::SkipEmptyParts);
    
    if (lines.isEmpty() || !lines[0].startsWith("200")) {
        emit folderShareFailed("Failed to initiate share: " + response);
        return;
    }
    
    // Parse session info
    QString session_id;
    int total_files = 0;
    QList<FileNodeInfo> files;
    bool inFiles = false;
    
    for (const QString &line : lines) {
        if (line.startsWith("SESSION_ID:")) {
            session_id = line.mid(11).trimmed();
        } else if (line.startsWith("TOTAL_FILES:")) {
            total_files = line.mid(12).trimmed().toInt();
        } else if (line.startsWith("FILES:")) {
            inFiles = true;
            continue;
        }
        
        if (inFiles && line.contains('|')) {
            // Format: old_file_id|name|relative_path|size_bytes
            QStringList parts = line.split('|');
            if (parts.size() >= 4) {
                FileNodeInfo info;
                info.file_id = parts[0].toLongLong();
                info.name = parts[1];
                info.relative_path = parts[2];
                info.size = parts[3].toLongLong();
                info.is_folder = false; // Files only
                files.append(info);
            }
        }
    }
    
    if (session_id.isEmpty()) {
        emit folderShareFailed("Invalid session ID from server");
        return;
    }
    
    // Save session info
    currentFolderShare.session_id = session_id;
    currentFolderShare.folder_id = folder_id;
    currentFolderShare.total_files = total_files;
    currentFolderShare.completed_files = 0;
    currentFolderShare.files_to_upload = files;
    isFolderShareActive = true;
    
    qDebug() << "[Network] Share initiated - Session:" << session_id 
             << ", Files:" << total_files;
    
    emit folderShareInitiated(session_id, total_files, files);
}

void NetworkManager::uploadFolderFile(const QString &session_id, 
                                     const FileNodeInfo &fileInfo, 
                                     const QString &localBasePath) {
    if (socket->state() != QAbstractSocket::ConnectedState) {
        emit folderShareFailed("Connection lost!");
        return;
    }
    
    // Construct local file path
    // localBasePath là đường dẫn đến folder gốc trên client
    // fileInfo.relative_path là đường dẫn tương đối từ folder gốc
    QString localFilePath = localBasePath + "/" + fileInfo.relative_path;
    
    qDebug() << "[Network] Uploading folder file:" << localFilePath;
    
    // Mở file
    QFile file(localFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit folderShareFailed("Cannot open file: " + localFilePath);
        return;
    }
    
    qint64 fileSize = file.size();
    
    // Disconnect readyRead
    disconnect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    // Gửi command: UPLOAD_FOLDER_FILE <session_id> <old_file_id> <file_size>
    QString cmd = QString("%1 %2 %3 %4\n")
                    .arg(CMD_UPLOAD_FOLDER_FILE)
                    .arg(session_id)
                    .arg(fileInfo.file_id)
                    .arg(fileSize);
    
    socket->write(cmd.toUtf8());
    socket->flush();
    
    // Đợi ACK (150 Ready to receive)
    if (!socket->waitForReadyRead(5000)) {
        file.close();
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        emit folderShareFailed("Timeout waiting for upload ACK");
        return;
    }
    
    QString ack = QString::fromUtf8(socket->readLine()).trimmed();
    qDebug() << "[Network] Upload ACK:" << ack;
    
    if (!ack.startsWith(CODE_DATA_OPEN)) {
        file.close();
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        emit folderShareFailed("Server rejected file upload: " + ack);
        return;
    }
    
    // Gửi binary data
    qint64 totalSent = 0;
    char buffer[65536]; // 64KB chunks
    
    while (!file.atEnd()) {
        qint64 bytesRead = file.read(buffer, sizeof(buffer));
        if (bytesRead == -1) {
            file.close();
            connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
            emit folderShareFailed("Error reading file: " + localFilePath);
            return;
        }
        
        qint64 bytesWritten = socket->write(buffer, bytesRead);
        if (bytesWritten == -1) {
            file.close();
            connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
            emit folderShareFailed("Socket write error");
            return;
        }
        
        socket->waitForBytesWritten(100);
        totalSent += bytesWritten;
    }
    
    socket->flush();
    file.close();
    
    qDebug() << "[Network] File data sent:" << totalSent << "bytes";
    
    // Đợi response (202 progress hoặc 226 complete)
    if (!socket->waitForReadyRead(10000)) {
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        emit folderShareFailed("Timeout waiting for upload confirmation");
        return;
    }
    
    QString response = QString::fromUtf8(socket->readAll()).trimmed();
    qDebug() << "[Network] Upload response:" << response;
    
    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    // Parse response
    if (response.startsWith("202")) {
        // Still in progress
        // Parse: "202 STATUS:uploading|COMPLETED:3|TOTAL:5|PROGRESS:60%"
        QStringList parts = response.split('|');
        int completed = 0;
        int total = 0;
        
        for (const QString &part : parts) {
            if (part.contains("COMPLETED:")) {
                completed = part.split(':')[1].toInt();
            } else if (part.contains("TOTAL:")) {
                total = part.split(':')[1].toInt();
            }
        }
        
        currentFolderShare.completed_files = completed;
        emit folderFileUploaded(completed, total);
        
        int percentage = (total > 0) ? (completed * 100 / total) : 0;
        emit folderShareProgress(percentage, QString("Uploading files: %1/%2").arg(completed).arg(total));
        
    } else if (response.startsWith(CODE_TRANSFER_COMPLETE)) {
        // Complete!
        emit folderShareCompleted(session_id);
        emit folderShareProgress(100, "Folder share completed!");
        
        isFolderShareActive = false;
        currentFolderShare = FolderShareSessionInfo();
        
        // Refresh file list
        QTimer::singleShot(500, this, &NetworkManager::requestFileList);
        
    } else {
        emit folderShareFailed("Upload failed: " + response);
    }
}

void NetworkManager::checkShareProgress(const QString &session_id) {
    if (socket->state() != QAbstractSocket::ConnectedState) {
        return;
    }
    
    disconnect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    QString cmd = QString("%1 %2\n").arg(CMD_CHECK_SHARE_PROGRESS).arg(session_id);
    socket->write(cmd.toUtf8());
    socket->flush();
    
    if (!socket->waitForReadyRead(3000)) {
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        return;
    }
    
    QString response = QString::fromUtf8(socket->readAll()).trimmed();
    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    qDebug() << "[Network] Progress check:" << response;
    
    // Parse progress
    if (response.startsWith("200")) {
        // Parse: "200 STATUS:uploading|COMPLETED:3|TOTAL:5|PROGRESS:60%"
        QStringList parts = response.split('|');
        int completed = 0;
        int total = 0;
        QString status;
        
        for (const QString &part : parts) {
            if (part.contains("STATUS:")) {
                status = part.split(':')[1];
            } else if (part.contains("COMPLETED:")) {
                completed = part.split(':')[1].toInt();
            } else if (part.contains("TOTAL:")) {
                total = part.split(':')[1].toInt();
            }
        }
        
        int percentage = (total > 0) ? (completed * 100 / total) : 0;
        emit folderShareProgress(percentage, QString("%1: %2/%3").arg(status).arg(completed).arg(total));
    }
}

void NetworkManager::cancelFolderShare(const QString &session_id) {
    if (socket->state() != QAbstractSocket::ConnectedState) {
        return;
    }
    
    QString cmd = QString("%1 %2\n").arg(CMD_CANCEL_FOLDER_SHARE).arg(session_id);
    socket->write(cmd.toUtf8());
    socket->flush();
    
    isFolderShareActive = false;
    currentFolderShare = FolderShareSessionInfo();
    
    qDebug() << "[Network] Cancelled folder share:" << session_id;
}

void NetworkManager::createFolder(const QString &folderName, long long parent_id) {
    if (socket->state() != QAbstractSocket::ConnectedState) {
        emit folderCreated(false, "Not connected to server!", -1);
        return;
    }
    
    qDebug() << "[Network] Creating folder:" << folderName << "in parent:" << parent_id;
    
    disconnect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    // Send command: MKDIR <folder_name> <parent_id>
    QString cmd = QString("%1 %2 %3\n")
                    .arg(CMD_CREATE_FOLDER)
                    .arg(folderName)
                    .arg(parent_id);
    
    socket->write(cmd.toUtf8());
    socket->flush();
    
    // Wait for response
    if (!socket->waitForReadyRead(5000)) {
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        emit folderCreated(false, "Timeout waiting for server response", -1);
        return;
    }
    
    QString response = QString::fromUtf8(socket->readAll()).trimmed();
    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    qDebug() << "[Network] Create folder response:" << response;
    
    // Parse response: "200 Folder created successfully (ID: 123)"
    if (response.startsWith(CODE_OK)) {
        // Extract folder_id from response
        long long folder_id = -1;
        QRegularExpression regex("ID: (\\d+)");
        QRegularExpressionMatch match = regex.match(response);
        if (match.hasMatch()) {
            folder_id = match.captured(1).toLongLong();
        }
        
        emit folderCreated(true, folderName, folder_id);
        
        // Refresh file list after 200ms
        QTimer::singleShot(200, this, &NetworkManager::requestFileList);
    } else {
        emit folderCreated(false, response, -1);
    }
}