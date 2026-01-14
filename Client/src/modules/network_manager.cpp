#include "network_manager.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QTimer>
#include <QThread>
#include <arpa/inet.h>
#include <endian.h>

NetworkManager::NetworkManager(QObject *parent) : QObject(parent) {
    socket = new QTcpSocket(this);
    
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

void NetworkManager::login(const QString &user, const QString &pass) {
    qDebug() << "[CLIENT] ===== LOGIN ATTEMPT =====";
    qDebug() << "[CLIENT] Cmd: USER" << user;
    
    if(socket->state() != QAbstractSocket::ConnectedState) {
        qDebug() << "[CLIENT] Login FAILED: Not connected to server";
        emit loginFailed("Not connected to server!");
        return;
    }
    
    disconnect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    QString cmdUser = QString("%1 %2\n").arg(CMD_USER, user);
    socket->write(cmdUser.toUtf8());
    socket->flush();
    
    if (!socket->waitForReadyRead(3000)) {
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        emit loginFailed("Timeout: Server not responding");
        return;
    }
    
    QString userResponse = QString::fromUtf8(socket->readAll()).trimmed();
    
    if (!userResponse.startsWith("331")) {
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        emit loginFailed("Unexpected server response: " + userResponse);
        return;
    }
    
    QString cmdPass = QString("%1 %2\n").arg(CMD_PASS, pass);
    socket->write(cmdPass.toUtf8());
    socket->flush();
    
    if (!socket->waitForReadyRead(3000)) {
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        emit loginFailed("Timeout: Server not responding to password");
        return;
    }
    
    QString passResponse = QString::fromUtf8(socket->readAll()).trimmed();
    
    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    if (passResponse.startsWith(CODE_LOGIN_SUCCESS)) {
        currentUsername = user;
        currentPassword = pass;
        qDebug() << "[CLIENT] Login SUCCESS for user:" << user;
        emit loginSuccess();
    } else if (passResponse.startsWith(CODE_LOGIN_FAIL)) {
        qDebug() << "[CLIENT] Login FAILED: Invalid username or password";
        emit loginFailed("Invalid username or password");
    } else {
        qDebug() << "[CLIENT] Login FAILED: Unexpected response -" << passResponse;
        emit loginFailed("Unexpected server response: " + passResponse);
    }
}
void NetworkManager::registerAccount(const QString &user, const QString &pass) {
    qDebug() << "[CLIENT] ===== REGISTER ATTEMPT =====";
    qDebug() << "[CLIENT] Cmd: REGISTER" << user;
    
    if(socket->state() != QAbstractSocket::ConnectedState) {
        qDebug() << "[CLIENT] Register FAILED: Not connected to server";
        emit registerFailed("Not connected to server!");
        return;
    }

    disconnect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    QString cmdRegister = QString("%1 %2 %3\n").arg(CMD_REGISTER, user, pass);
    socket->write(cmdRegister.toUtf8());
    socket->flush();
    
    if (!socket->waitForReadyRead(5000)) {
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        emit registerFailed("Timeout: Server not responding");
        return;
    }
    
    QString response = QString::fromUtf8(socket->readAll()).trimmed();
    
    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    if (response.startsWith(CODE_OK)) {
        emit registerSuccess("Registration successful! You can now login.");
    } else if (response.startsWith(CODE_FAIL)) {
        QString errorMsg = "Registration failed";
        if (response.contains("already exists")) {
            errorMsg = "Username already exists!";
        } else if (response.contains("invalid")) {
            errorMsg = "Invalid username or password format!";
        } else {
            errorMsg = response;
        }
        emit registerFailed(errorMsg);
    } else {
        emit registerFailed("Unexpected server response: " + response);
    }
}
void NetworkManager::requestFileList(long long parent_id) {
    currentParentId = parent_id;
    QString cmd = QString("%1 %2\n").arg(CMD_LIST).arg(parent_id);
    socket->write(cmd.toUtf8());
    socket->flush();
}

void NetworkManager::requestSharedFileList(long long parent_id) {
    QString cmd;
    if (parent_id < 0) {
        cmd = QString("%1\n").arg(CMD_LISTSHARED);
    } else {
        cmd = QString("%1 %2\n").arg(CMD_LISTSHARED).arg(parent_id);
    }
    socket->write(cmd.toUtf8());
    socket->flush();
}

void NetworkManager::uploadFile(const QString &filePath, long long parent_id) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit uploadProgress("Failed to open file!");
        return;
    }

    QFileInfo fileInfo(filePath);
    QString filename = fileInfo.fileName();
    qint64 filesize = fileInfo.size();
    
    qDebug() << "[CLIENT] ===== UPLOAD FILE =====";
    qDebug() << "[CLIENT] Cmd: STOR" << filename << "size:" << filesize << "parent_id:" << parent_id;
    
    currentParentId = parent_id;

    emit uploadStarted(filename);
    emit transferProgress(0, filesize);

    disconnect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    auto handleError = [&](const QString &msg) {
        file.close();
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        emit uploadProgress(msg);
    };

    QString checkCmd = QString("%1 %2 %3\n").arg(CMD_UPLOAD_CHECK).arg(filename).arg(filesize);
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

    QString storCmd = QString("%1 %2 %3 %4\n").arg(CMD_UPLOAD).arg(filename).arg(filesize).arg(parent_id);
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

    qint64 totalSent = 0;
    char buffer[65536];
    const qint64 ACK_GROUP_SIZE = 1048576;
    qint64 bytesSinceLastAck = 0;
    int chunkCount = 0;

    emit transferProgress(0, filesize);

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
        
        socket->waitForBytesWritten(100);
        totalSent += bytesWritten;
        bytesSinceLastAck += bytesWritten;
        chunkCount++;
        
        emit transferProgress(totalSent, filesize);
        
        if (bytesSinceLastAck >= ACK_GROUP_SIZE) {
            socket->flush();
            if (!socket->waitForReadyRead(3000)) {
                handleError("Timeout waiting for chunk ACK.");
                return;
            }
            QString ack = QString::fromUtf8(socket->readAll()).trimmed();
            if (!ack.startsWith(CODE_CHUNK_ACK)) {
                handleError("Invalid chunk ACK: " + ack);
                return;
            }
            bytesSinceLastAck = 0;
            chunkCount = 0;
        }
    }
    
    socket->flush();
    file.close();

    if (!socket->waitForReadyRead(15000)) {
        handleError("Timeout waiting for server confirmation.");
        return;
    }
    
    response = QString::fromUtf8(socket->readAll()).trimmed();
    
    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    if (response.startsWith(CODE_TRANSFER_COMPLETE)) {
        qDebug() << "[CLIENT] Upload SUCCESS:" << filename;
        emit uploadProgress("Upload successful: " + filename);
        QTimer::singleShot(200, this, [this]() {
            requestFileList(currentParentId);
        });
    } else {
        qDebug() << "[CLIENT] Upload FAILED:" << response;
        emit uploadProgress("Upload finished but server reported error: " + response);
    }
}

void NetworkManager::uploadFolder(const QString &folderPath, long long parent_id) {
    QDir folder(folderPath);
    if (!folder.exists()) {
        emit uploadProgress("Folder does not exist!");
        return;
    }
    
    QFileInfo folderInfo(folderPath);
    QString folderName = folderInfo.fileName();
    
    qDebug() << "[CLIENT] ===== UPLOAD FOLDER =====";
    qDebug() << "[CLIENT] Cmd: CREATE_FOLDER" << folderName << "parent_id:" << parent_id;
    
    disconnect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    // Step 1: Create root folder on server
    QString createFolderCmd = QString("CREATE_FOLDER %1 %2\n").arg(folderName).arg(parent_id);
    socket->write(createFolderCmd.toUtf8());
    socket->flush();
    
    if (!socket->waitForReadyRead(5000)) {
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        emit uploadProgress("Timeout: Server not responding to create folder");
        return;
    }
    
    QString response = QString::fromUtf8(socket->readAll()).trimmed();
    if (!response.startsWith(CODE_OK)) {
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        emit uploadProgress("Failed to create folder: " + response);
        return;
    }
    
    // Extract folder ID from response "200 OK|FOLDER_ID:123"
    long long rootFolderId = parent_id;
    QStringList parts = response.split('|');
    for (const QString &part : parts) {
        if (part.contains("FOLDER_ID:")) {
            rootFolderId = part.split(':')[1].toLongLong();
            break;
        }
    }
    
    // Step 2: Collect folder structure and files
    struct FolderNode {
        QString relativePath;
        QString fullPath;
        bool isFolder;
        long long parentId;
        long long folderId;
    };
    
    QList<FolderNode> items;
    QMap<QString, long long> folderIdMap; // relativePath -> folder_id
    folderIdMap[""] = rootFolderId;
    
    std::function<void(const QString&, const QString&, long long)> collectItems = [&](const QString &basePath, const QString &relPath, long long parentFolderId) {
        QDir dir(basePath);
        QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        
        for (const QFileInfo &entry : entries) {
            QString newRelPath = relPath.isEmpty() ? entry.fileName() : relPath + "/" + entry.fileName();
            
            FolderNode node;
            node.relativePath = newRelPath;
            node.fullPath = entry.absoluteFilePath();
            node.isFolder = entry.isDir();
            node.parentId = parentFolderId;
            node.folderId = -1;
            
            items.append(node);
            
            if (entry.isDir()) {
                collectItems(entry.absoluteFilePath(), newRelPath, -1); // Will update parentId later
            }
        }
    };
    
    collectItems(folderPath, "", rootFolderId);
    
    // Count files for progress
    int fileCount = 0;
    for (const auto &item : items) {
        if (!item.isFolder) fileCount++;
    }
    
    if (fileCount == 0) {
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        emit uploadProgress("No files found in folder!");
        return;
    }
    
    emit folderUploadStarted(folderName, fileCount);
    
    int uploadedCount = 0;
    bool uploadSuccess = true;
    
    // Step 3: Process items (folders first, then files)
    for (auto &item : items) {
        if (item.isFolder) {
            // Get parent folder ID
            QString parentPath = item.relativePath;
            int lastSlash = parentPath.lastIndexOf('/');
            QString parentRelPath = lastSlash > 0 ? parentPath.left(lastSlash) : "";
            long long parentId = folderIdMap.value(parentRelPath, rootFolderId);
            
            // Create subfolder
            QString createSubfolderCmd = QString("CREATE_FOLDER %1 %2\n").arg(item.relativePath.section('/', -1)).arg(parentId);
            socket->write(createSubfolderCmd.toUtf8());
            socket->flush();
            
            if (!socket->waitForReadyRead(5000)) {
                uploadSuccess = false;
                emit uploadProgress("Timeout creating folder: " + item.relativePath);
                break;
            }
            
            response = QString::fromUtf8(socket->readAll()).trimmed();
            if (!response.startsWith(CODE_OK)) {
                uploadSuccess = false;
                emit uploadProgress("Failed to create folder: " + item.relativePath);
                break;
            }
            
            // Extract subfolder ID
            long long subfolderId = parentId;
            parts = response.split('|');
            for (const QString &part : parts) {
                if (part.contains("FOLDER_ID:")) {
                    subfolderId = part.split(':')[1].toLongLong();
                    break;
                }
            }
            
            folderIdMap[item.relativePath] = subfolderId;
            item.folderId = subfolderId;
        }
    }
    
    if (!uploadSuccess) {
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        return;
    }
    
    // Step 4: Upload files
    for (const auto &item : items) {
        if (item.isFolder) continue;
        
        QString fullPath = item.fullPath;
        QString relativePath = item.relativePath;
        
        // Get parent folder ID for this file
        QString parentPath = relativePath;
        int lastSlash = parentPath.lastIndexOf('/');
        QString parentRelPath = lastSlash > 0 ? parentPath.left(lastSlash) : "";
        long long fileParentId = folderIdMap.value(parentRelPath, rootFolderId);
        
        QFile file(fullPath);
        if (!file.open(QIODevice::ReadOnly)) {
            emit folderUploadProgress(uploadedCount, fileCount, relativePath + " (failed to open)");
            continue;
        }
        
        QFileInfo fileInfo(fullPath);
        QString fileName = fileInfo.fileName();
        qint64 filesize = fileInfo.size();
        
        emit folderUploadProgress(uploadedCount, fileCount, relativePath);
        
        // Send upload command (just filename, not path)
        QString uploadCmd = QString("%1 %2 %3 %4\n").arg(CMD_UPLOAD).arg(fileName).arg(filesize).arg(fileParentId);
        socket->write(uploadCmd.toUtf8());
        socket->flush();
        
        if (!socket->waitForReadyRead(5000)) {
            file.close();
            uploadSuccess = false;
            emit uploadProgress("Timeout during folder upload: " + relativePath);
            break;
        }
        
        QString response = QString::fromUtf8(socket->readAll()).trimmed();
        if (!response.startsWith(CODE_DATA_OPEN)) {
            file.close();
            uploadSuccess = false;
            emit uploadProgress("Server rejected file: " + relativePath);
            break;
        }
        
        // Upload file data
        char buffer[65536];
        qint64 totalSent = 0;
        
        while (!file.atEnd()) {
            if (socket->state() != QAbstractSocket::ConnectedState) {
                file.close();
                uploadSuccess = false;
                emit uploadProgress("Network disconnected during upload!");
                break;
            }
            
            qint64 bytesRead = file.read(buffer, sizeof(buffer));
            if (bytesRead == -1) {
                file.close();
                uploadSuccess = false;
                break;
            }
            
            qint64 bytesWritten = socket->write(buffer, bytesRead);
            if (bytesWritten == -1) {
                file.close();
                uploadSuccess = false;
                break;
            }
            
            socket->waitForBytesWritten(100);
            totalSent += bytesWritten;
        }
        
        file.close();
        
        if (!uploadSuccess) {
            break;
        }
        
        socket->flush();
        
        if (!socket->waitForReadyRead(10000)) {
            uploadSuccess = false;
            emit uploadProgress("Timeout waiting for confirmation: " + relativePath);
            break;
        }
        
        response = QString::fromUtf8(socket->readAll()).trimmed();
        if (!response.startsWith(CODE_TRANSFER_COMPLETE)) {
            uploadSuccess = false;
            emit uploadProgress("Upload failed for: " + relativePath);
            break;
        }
        
        uploadedCount++;
    }
    
    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    if (uploadSuccess) {
        qDebug() << "[CLIENT] Upload Folder SUCCESS:" << folderName << "(" << uploadedCount << "files)";
        emit folderUploadCompleted(folderName);
        emit uploadProgress("Folder uploaded successfully: " + folderName);
        QTimer::singleShot(200, this, [this]() {
            requestFileList(currentParentId);
        });
    } else {
        qDebug() << "[CLIENT] Upload Folder FAILED: Some files failed to upload";
        emit uploadProgress("Folder upload incomplete. Some files may have failed.");
    }
}

void NetworkManager::downloadFile(const QString &filename, const QString &savePath) {
    qDebug() << "[CLIENT] ===== DOWNLOAD FILE =====";
    qDebug() << "[CLIENT] Cmd: RETR" << filename;
    
    if(socket->state() != QAbstractSocket::ConnectedState) {
        qDebug() << "[CLIENT] Download FAILED: Not connected to server";
        emit downloadComplete("Not connected to server!");
        return;
    }
    
    emit downloadStarted(filename);
    emit transferProgress(0, 1);
    
    disconnect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    auto handleDownloadError = [&](const QString &msg, QFile &f) {
        f.close();
        f.remove();
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        emit downloadComplete(msg);
    };

    QString cmd = QString("%1 %2\n").arg(CMD_DOWNLOAD, filename);
    socket->write(cmd.toUtf8());
    socket->flush();
    
    if (!socket->waitForReadyRead(5000)) {
        QFile dummy;
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        emit downloadComplete("Timeout: Server not responding to download request.");
        return;
    }

    QString response = QString::fromUtf8(socket->readLine()).trimmed();
    
    if (!response.startsWith(CODE_DATA_OPEN)) {
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        emit downloadComplete("Download failed: " + response);
        return;
    }

    QStringList parts = response.split(' ');
    qint64 filesize = 0;
    if (parts.size() >= 2) {
        filesize = parts[1].toLongLong();
    }
    
    if (filesize == 0) {
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        emit downloadComplete("Error: File is empty or invalid response from server");
        return;
    }
    
    QFile file(savePath);
    if (!file.open(QIODevice::WriteOnly)) {
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        emit downloadComplete("Cannot save file (Permission denied?): " + savePath);
        return;
    }

    emit transferProgress(0, filesize);

    qint64 totalReceived = 0;
    int retryCount = 0;
    const qint64 ACK_GROUP_SIZE = 1048576;
    qint64 bytesSinceLastAck = 0;

    while (totalReceived < filesize) {
        if (!socket->waitForReadyRead(5000)) {
            if (socket->state() != QAbstractSocket::ConnectedState) {
                handleDownloadError("Network connection lost!", file);
                return;
            }
            retryCount++;
            if (retryCount > 3) {
                 handleDownloadError("Data transfer timeout.", file);
                 return;
            }
            continue;
        }
        
        retryCount = 0;

        // Only read the remaining bytes needed, not more
        qint64 remainingBytes = filesize - totalReceived;
        qint64 bytesToRead = qMin(socket->bytesAvailable(), remainingBytes);
        QByteArray chunk = socket->read(bytesToRead);
        qint64 bytesWritten = file.write(chunk);
        
        if (bytesWritten == -1) {
            handleDownloadError("Disk full or write error!", file);
            return;
        }

        totalReceived += bytesWritten;
        bytesSinceLastAck += bytesWritten;
        
        emit transferProgress(totalReceived, filesize);
        
        if (bytesSinceLastAck >= ACK_GROUP_SIZE && totalReceived < filesize) {
            QString ack = QString("%1 Received %2 bytes\n").arg(CODE_CHUNK_ACK).arg(totalReceived);
            socket->write(ack.toUtf8());
            socket->flush();
            bytesSinceLastAck = 0;
        }
    }
    
    file.close();

    if (socket->bytesAvailable() > 0 || socket->waitForReadyRead(2000)) {
        QString finalMsg = QString::fromUtf8(socket->readLine()).trimmed();
    }
    
    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    if (totalReceived == filesize) {
        emit downloadComplete("File saved successfully to: " + savePath);
    } else {
        emit downloadComplete("Warning: File size mismatch. Downloaded: " + QString::number(totalReceived));
    }
}

void NetworkManager::shareFile(const QString &filename, const QString &targetUser) {
    qDebug() << "[CLIENT] ===== SHARE FILE =====";
    qDebug() << "[CLIENT] Cmd: SHARE" << filename << "to" << targetUser;
    qDebug() << "[NetworkManager::shareFile] Called with filename:" << filename << "targetUser:" << targetUser;
    qDebug() << "[NetworkManager::shareFile] Socket state:" << socket->state();
    
    if(socket->state() != QAbstractSocket::ConnectedState) {
        qDebug() << "[NetworkManager::shareFile] Socket not connected! State:" << socket->state();
        emit shareResult(false, "Not connected!");
        return;
    }

    QString cmd = QString("SHARE %1 %2\n").arg(filename, targetUser);
    qDebug() << "[NetworkManager::shareFile] Sending command:" << cmd.trimmed();
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
    qDebug() << "[CLIENT] ===== DELETE FILE =====";
    qDebug() << "[CLIENT] Cmd: DELETE" << filename;
    
    if(socket->state() != QAbstractSocket::ConnectedState) {
        qDebug() << "[CLIENT] Delete FAILED: Not connected to server";
        emit deleteResult(false, "Not connected!");
        return;
    }

    QString cmd = QString("DELETE %1\n").arg(filename);
    socket->write(cmd.toUtf8());
    socket->flush();
    
    disconnect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    if (!socket->waitForReadyRead(5000)) {
        qDebug() << "[CLIENT] Delete FAILED: Timeout waiting for response";
        emit deleteResult(false, "Timeout waiting for delete response.");
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        return;
    }
    
    QString response = QString::fromUtf8(socket->readAll()).trimmed();
    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    if (response.startsWith(CODE_OK)) {
        qDebug() << "[CLIENT] Delete SUCCESS:" << filename;
        emit deleteResult(true, "File deleted successfully!");
    } else {
        qDebug() << "[CLIENT] Delete FAILED:" << response;
        emit deleteResult(false, "Delete failed: " + response);
    }
}

void NetworkManager::renameItem(const QString &fileId, const QString &newName, const QString &itemType) {
    qDebug() << "[CLIENT] ===== RENAME ITEM =====";
    qDebug() << "[CLIENT] Cmd: RENAME" << itemType << "ID:" << fileId << "to" << newName;
    qDebug() << "[NetworkManager::renameItem] Called with fileId:" << fileId << "newName:" << newName << "itemType:" << itemType;
    qDebug() << "[NetworkManager::renameItem] Socket state:" << socket->state();
    
    if(socket->state() != QAbstractSocket::ConnectedState) {
        qDebug() << "[NetworkManager::renameItem] Socket not connected! State:" << socket->state();
        emit renameResult(false, "Not connected!");
        return;
    }

    QString cmd = QString("%1 %2 %3\n").arg(CMD_RENAME, fileId, newName);
    qDebug() << "[NetworkManager::renameItem] Sending command:" << cmd.trimmed();
    socket->write(cmd.toUtf8());
    socket->flush();
    
    disconnect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    if (!socket->waitForReadyRead(5000)) {
        emit renameResult(false, "Timeout waiting for rename response.");
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        return;
    }
    
    QString response = QString::fromUtf8(socket->readAll()).trimmed();
    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    if (response.startsWith(CODE_OK)) {
        emit renameResult(true, QString("%1 renamed successfully!").arg(itemType));
    } else {
        emit renameResult(false, "Rename failed: " + response);
    }
}

void NetworkManager::downloadFolder(long long folder_id, const QString &folderName, const QString &savePath) {
    if(socket->state() != QAbstractSocket::ConnectedState) {
        emit downloadComplete("Not connected to server!");
        return;
    }
    
    emit downloadStarted(folderName);
    
    disconnect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    // Gửi folder_id thay vì folder_name để server có thể query database
    QString cmd = QString("%1 %2\n").arg(CMD_DOWNLOAD_FOLDER).arg(folder_id);
    socket->write(cmd.toUtf8());
    socket->flush();
    
    if (!socket->waitForReadyRead(5000)) {
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        emit downloadComplete("Timeout: Server not responding");
        return;
    }
    
    QString response = QString::fromUtf8(socket->readLine()).trimmed();
    
    if (!response.startsWith(CODE_DATA_OPEN)) {
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        emit downloadComplete("Download failed: " + response);
        return;
    }
    
    // Create base folder
    QDir().mkpath(savePath);
    
    qDebug() << "[NetworkManager] Starting to receive folder structure...";
    
    // Receive folder structure
    while (true) {
        // Wait for data with timeout
        while (socket->bytesAvailable() < 1) {
            if (!socket->waitForReadyRead(10000)) {
                connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
                emit downloadComplete("Timeout while receiving folder data");
                return;
            }
        }
        
        // Read type
        char typeChar;
        qint64 n = socket->read(&typeChar, 1);
        if (n != 1) {
            connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
            emit downloadComplete("Error reading type");
            return;
        }
        uint8_t type = static_cast<uint8_t>(typeChar);
        
        qDebug() << "[NetworkManager] Received type:" << type;
        
        if (type == TYPE_END) {
            qDebug() << "[NetworkManager] Received TYPE_END, download complete";
            break;
        }
        
        // Read name length
        uint32_t nameLen;
        qint64 bytesRead = 0;
        while (bytesRead < 4) {
            // Check if data is already available
            if (socket->bytesAvailable() == 0) {
                if (!socket->waitForReadyRead(5000)) {
                    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
                    qDebug() << "[NetworkManager] Timeout reading name length";
                    emit downloadComplete("Timeout reading name length");
                    return;
                }
            }
            qint64 n = socket->read(reinterpret_cast<char*>(&nameLen) + bytesRead, 4 - bytesRead);
            if (n <= 0) {
                connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
                qDebug() << "[NetworkManager] Error reading name length";
                emit downloadComplete("Error reading name length");
                return;
            }
            bytesRead += n;
        }
        nameLen = ntohl(nameLen);
        qDebug() << "[NetworkManager] Name length:" << nameLen;
        
        // Read file size if it's a file
        uint64_t fileSize = 0;
        if (type == TYPE_FILE) {
            qint64 sizeRead = 0;
            while (sizeRead < 8) {
                // Check if data is already available
                if (socket->bytesAvailable() == 0) {
                    if (!socket->waitForReadyRead(5000)) {
                        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
                        qDebug() << "[NetworkManager] Timeout reading file size, bytes available:" << socket->bytesAvailable();
                        emit downloadComplete("Timeout reading file size");
                        return;
                    }
                }
                qint64 n = socket->read(reinterpret_cast<char*>(&fileSize) + sizeRead, 8 - sizeRead);
                if (n <= 0) {
                    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
                    qDebug() << "[NetworkManager] Error reading file size, read returned:" << n;
                    emit downloadComplete("Error reading file size");
                    return;
                }
                sizeRead += n;
            }
            fileSize = be64toh(fileSize);
            qDebug() << "[NetworkManager] File size:" << fileSize;
        }
        
        // Read name
        QByteArray nameData;
        while (nameData.size() < static_cast<int>(nameLen)) {
            qint64 remaining = nameLen - nameData.size();
            qint64 available = socket->bytesAvailable();
            
            if (available > 0) {
                // Đọc dữ liệu có sẵn trong buffer
                nameData.append(socket->read(qMin(remaining, available)));
            } else {
                // Chờ thêm dữ liệu
                if (!socket->waitForReadyRead(5000)) {
                    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
                    qDebug() << "[NetworkManager] Timeout reading name, got" << nameData.size() << "of" << nameLen << "bytes";
                    emit downloadComplete("Timeout reading name");
                    return;
                }
            }
        }
        QString name = QString::fromUtf8(nameData);
        
        qDebug() << "[NetworkManager] Received name:" << name << "type:" << type;
        
        QString fullPath = savePath + "/" + name;
        
        if (type == TYPE_DIR) {
            QDir().mkpath(fullPath);
            qDebug() << "[NetworkManager] Created directory:" << fullPath;
        } else if (type == TYPE_FILE) {
            // Ensure parent directory exists
            QFileInfo fileInfo(fullPath);
            QDir().mkpath(fileInfo.absolutePath());
            
            QFile file(fullPath);
            if (!file.open(QIODevice::WriteOnly)) {
                connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
                emit downloadComplete("Cannot create file: " + fullPath);
                return;
            }
            
            // Receive file data
            uint64_t received = 0;
            qDebug() << "[NetworkManager] Receiving file:" << name << "size:" << fileSize;
            while (received < fileSize) {
                qint64 available = socket->bytesAvailable();
                if (available == 0) {
                    if (!socket->waitForReadyRead(30000)) {
                        file.close();
                        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
                        qDebug() << "[NetworkManager] Timeout receiving file data, received:" << received << "of" << fileSize;
                        emit downloadComplete("Timeout receiving file data");
                        return;
                    }
                    available = socket->bytesAvailable();
                }
                
                // Đọc đúng số bytes cần thiết, không đọc quá fileSize - received
                qint64 toRead = qMin(available, static_cast<qint64>(fileSize - received));
                if (toRead <= 0) break;
                
                QByteArray chunk = socket->read(toRead);
                if (chunk.isEmpty()) {
                    continue;
                }
                file.write(chunk);
                received += chunk.size();
            }
            
            file.close();
            qDebug() << "[NetworkManager] File saved:" << fullPath << "(" << received << "bytes)";
        }
    }
    
    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    emit downloadComplete("Folder downloaded successfully: " + folderName);
}

void NetworkManager::logout() {
    if (socket->state() == QAbstractSocket::ConnectedState) {
        socket->disconnectFromHost();
    }
    currentUsername.clear();
    currentPassword.clear();
    emit logoutSuccess();
}

void NetworkManager::onReadyRead() {
    QByteArray allData = socket->readAll();
    QString fullResponse = QString::fromUtf8(allData).trimmed();
    
    if (fullResponse.isEmpty()) return;
    
    QStringList lines = fullResponse.split('\n', Qt::SkipEmptyParts);
    
    QStringList fileListLines;
    bool hasLoginSuccess = false;
    bool hasLoginFailed = false;
    
    for (const QString &line : lines) {
        QString trimmed = line.trimmed();
        
        if (trimmed.startsWith(CODE_LOGIN_SUCCESS)) {
            hasLoginSuccess = true;
        } 
        else if (trimmed.startsWith(CODE_LOGIN_FAIL)) {
            hasLoginFailed = true;
        }
        else if (trimmed.startsWith("210")) {
            emit fileListReceived("");
            return;
        }
        else if (trimmed.contains('|')) {
            fileListLines.append(trimmed);
        }
    }
    
    if (hasLoginSuccess) {
        emit loginSuccess();
    }
    if (hasLoginFailed) {
        emit loginFailed("Invalid Username or Password");
    }
    
    if (!fileListLines.isEmpty()) {
        QString fileListData = fileListLines.join("\n");
        emit fileListReceived(fileListData);
    }
}
void NetworkManager::getFolderStructure(long long folder_id) {
    if (socket->state() != QAbstractSocket::ConnectedState) {
        emit folderShareFailed("Not connected to server!");
        return;
    }
    
    disconnect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    QString cmd = QString("%1 %2\n").arg(CMD_GET_FOLDER_STRUCTURE).arg(folder_id);
    socket->write(cmd.toUtf8());
    socket->flush();
    
    if (!socket->waitForReadyRead(5000)) {
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        emit folderShareFailed("Timeout getting folder structure");
        return;
    }
    
    QString response;
    while (socket->canReadLine()) {
        response += QString::fromUtf8(socket->readLine());
    }
    
    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    QStringList lines = response.split('\n', Qt::SkipEmptyParts);
    
    if (lines.isEmpty() || !lines[0].startsWith("200")) {
        emit folderShareFailed("Failed to get folder structure: " + response);
        return;
    }
    
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
    
    emit folderStructureReceived(folder_id, structure);
}

void NetworkManager::shareFolderRequest(long long folder_id, const QString &targetUser) {
    qDebug() << "[NetworkManager::shareFolderRequest] Called with folder_id:" << folder_id << "targetUser:" << targetUser;
    qDebug() << "[NetworkManager::shareFolderRequest] Socket state:" << socket->state();
    
    if (socket->state() != QAbstractSocket::ConnectedState) {
        qDebug() << "[NetworkManager::shareFolderRequest] Socket not connected! State:" << socket->state();
        emit folderShareFailed("Not connected to server!");
        return;
    }
    
    disconnect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    QString cmd = QString("%1 %2 %3\n").arg(CMD_SHARE_FOLDER).arg(folder_id).arg(targetUser);
    qDebug() << "[NetworkManager::shareFolderRequest] Sending command:" << cmd.trimmed();
    socket->write(cmd.toUtf8());
    socket->flush();
    
    if (!socket->waitForReadyRead(10000)) {
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        emit folderShareFailed("Timeout initiating folder share");
        return;
    }
    
    QString response;
    while (socket->canReadLine()) {
        response += QString::fromUtf8(socket->readLine());
    }
    
    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    QStringList lines = response.split('\n', Qt::SkipEmptyParts);
    
    if (lines.isEmpty() || !lines[0].startsWith("200")) {
        emit folderShareFailed("Failed to initiate share: " + response);
        return;
    }
    
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
            QStringList parts = line.split('|');
            if (parts.size() >= 4) {
                FileNodeInfo info;
                info.file_id = parts[0].toLongLong();
                info.name = parts[1];
                info.relative_path = parts[2];
                info.size = parts[3].toLongLong();
                info.is_folder = false;
                files.append(info);
            }
        }
    }
    
    if (session_id.isEmpty()) {
        emit folderShareFailed("Invalid session ID from server");
        return;
    }
    
    currentFolderShare.session_id = session_id;
    currentFolderShare.folder_id = folder_id;
    currentFolderShare.total_files = total_files;
    currentFolderShare.completed_files = 0;
    currentFolderShare.files_to_upload = files;
    isFolderShareActive = true;
    
    emit folderShareInitiated(session_id, total_files, files);
}

void NetworkManager::uploadFolderFile(const QString &session_id, 
                                     const FileNodeInfo &fileInfo, 
                                     const QString &localBasePath) {
    if (socket->state() != QAbstractSocket::ConnectedState) {
        emit folderShareFailed("Connection lost!");
        return;
    }
    
    QString localFilePath = localBasePath + "/" + fileInfo.relative_path;
    
    QFile file(localFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        emit folderShareFailed("Cannot open file: " + localFilePath);
        return;
    }
    
    qint64 fileSize = file.size();
    
    disconnect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    QString cmd = QString("%1 %2 %3 %4\n")
                    .arg(CMD_UPLOAD_FILE)
                    .arg(session_id)
                    .arg(fileInfo.file_id)
                    .arg(fileSize);
    
    socket->write(cmd.toUtf8());
    socket->flush();
    
    if (!socket->waitForReadyRead(5000)) {
        file.close();
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        emit folderShareFailed("Timeout waiting for upload ACK");
        return;
    }
    
    QString ack = QString::fromUtf8(socket->readLine()).trimmed();
    
    if (!ack.startsWith(CODE_DATA_OPEN)) {
        file.close();
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        emit folderShareFailed("Server rejected file upload: " + ack);
        return;
    }
    
    qint64 totalSent = 0;
    char buffer[65536];
    
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
    
    if (!socket->waitForReadyRead(10000)) {
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        emit folderShareFailed("Timeout waiting for upload confirmation");
        return;
    }
    
    QString response = QString::fromUtf8(socket->readAll()).trimmed();
    
    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    if (response.startsWith("202")) {
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
        emit folderShareCompleted(session_id);
        emit folderShareProgress(100, "Folder share completed!");
        
        isFolderShareActive = false;
        currentFolderShare = FolderShareSessionInfo();
        
        QTimer::singleShot(500, this, [this]() {
            requestFileList(0);
        });
        
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
    
    if (response.startsWith("200")) {
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
}

// ===== SHARE CODE FUNCTIONS =====

void NetworkManager::generateShareCode(long long file_id, int max_uses) {
    if (socket->state() != QAbstractSocket::ConnectedState) {
        emit shareCodeGenerated(false, "", "Not connected to server!");
        return;
    }
    
    disconnect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    QString cmd = QString("%1 %2 %3\n").arg(CMD_GENERATE_SHARE_CODE).arg(file_id).arg(max_uses);
    socket->write(cmd.toUtf8());
    socket->flush();
    
    if (!socket->waitForReadyRead(5000)) {
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        emit shareCodeGenerated(false, "", "Timeout waiting for server response");
        return;
    }
    
    QString response = QString::fromUtf8(socket->readAll()).trimmed();
    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    if (response.startsWith(CODE_OK)) {
        QStringList parts = response.split(' ');
        if (parts.size() >= 2) {
            emit shareCodeGenerated(true, parts[1], "Share code generated!");
        } else {
            emit shareCodeGenerated(false, "", "Invalid server response");
        }
    } else {
        emit shareCodeGenerated(false, "", response);
    }
}

void NetworkManager::redeemShareCode(const QString &share_code) {
    if (socket->state() != QAbstractSocket::ConnectedState) {
        emit shareCodeRedeemed(false, -1, "", false, "");
        return;
    }
    
    disconnect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    QString cmd = QString("%1 %2\n").arg(CMD_REDEEM_SHARE_CODE).arg(share_code);
    socket->write(cmd.toUtf8());
    socket->flush();
    
    if (!socket->waitForReadyRead(5000)) {
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        emit shareCodeRedeemed(false, -1, "", false, "");
        return;
    }
    
    QString response = QString::fromUtf8(socket->readAll()).trimmed();
    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    if (response.startsWith(CODE_OK)) {
        // Format: 200 file_id|filename|is_folder|owner
        QStringList parts = response.mid(4).split('|');
        if (parts.size() >= 4) {
            long long file_id = parts[0].toLongLong();
            QString filename = parts[1];
            bool is_folder = (parts[2] == "1");
            QString owner = parts[3];
            emit shareCodeRedeemed(true, file_id, filename, is_folder, owner);
        } else {
            emit shareCodeRedeemed(false, -1, "", false, "");
        }
    } else {
        emit shareCodeRedeemed(false, -1, "", false, "");
    }
}

void NetworkManager::getMyShares() {
    if (socket->state() != QAbstractSocket::ConnectedState) {
        emit mySharesReceived(QList<ShareInfoClient>());
        return;
    }
    
    disconnect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    QString cmd = QString("%1\n").arg(CMD_GET_MY_SHARES);
    socket->write(cmd.toUtf8());
    socket->flush();
    
    if (!socket->waitForReadyRead(5000)) {
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        emit mySharesReceived(QList<ShareInfoClient>());
        return;
    }
    
    QString response = QString::fromUtf8(socket->readAll()).trimmed();
    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    QList<ShareInfoClient> shares;
    
    if (response.startsWith(CODE_OK)) {
        QStringList lines = response.split('\n');
        for (int i = 1; i < lines.size(); ++i) {
            QStringList parts = lines[i].split('|');
            if (parts.size() >= 7) {
                ShareInfoClient info;
                info.shared_id = parts[0].toLongLong();
                info.file_id = parts[1].toLongLong();
                info.filename = parts[2];
                info.is_folder = (parts[3] == "1");
                info.shared_with_username = parts[4];
                info.permission = parts[5];
                info.shared_at = parts[6];
                shares.append(info);
            }
        }
    }
    
    emit mySharesReceived(shares);
}

void NetworkManager::revokeShare(long long file_id, const QString &target_username) {
    if (socket->state() != QAbstractSocket::ConnectedState) {
        emit shareRevoked(false, "Not connected to server!");
        return;
    }
    
    disconnect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    QString cmd = QString("%1 %2 %3\n").arg(CMD_REVOKE_SHARE).arg(file_id).arg(target_username);
    socket->write(cmd.toUtf8());
    socket->flush();
    
    if (!socket->waitForReadyRead(5000)) {
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        emit shareRevoked(false, "Timeout waiting for server response");
        return;
    }
    
    QString response = QString::fromUtf8(socket->readAll()).trimmed();
    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    if (response.startsWith(CODE_OK)) {
        emit shareRevoked(true, "Share revoked successfully!");
    } else {
        emit shareRevoked(false, response);
    }
}

void NetworkManager::getMyShareCodes() {
    if (socket->state() != QAbstractSocket::ConnectedState) {
        emit myShareCodesReceived(QList<ShareCodeInfoClient>());
        return;
    }
    
    disconnect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    QString cmd = QString("%1\n").arg(CMD_GET_MY_SHARE_CODES);
    socket->write(cmd.toUtf8());
    socket->flush();
    
    if (!socket->waitForReadyRead(5000)) {
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        emit myShareCodesReceived(QList<ShareCodeInfoClient>());
        return;
    }
    
    QString response = QString::fromUtf8(socket->readAll()).trimmed();
    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    QList<ShareCodeInfoClient> codes;
    
    if (response.startsWith(CODE_OK)) {
        QStringList lines = response.split('\n');
        for (int i = 1; i < lines.size(); ++i) {
            QStringList parts = lines[i].split('|');
            if (parts.size() >= 8) {
                ShareCodeInfoClient info;
                info.code_id = parts[0].toLongLong();
                info.share_code = parts[1];
                info.file_id = parts[2].toLongLong();
                info.filename = parts[3];
                info.is_folder = (parts[4] == "1");
                info.max_uses = parts[5].toInt();
                info.current_uses = parts[6].toInt();
                info.created_at = parts[7];
                codes.append(info);
            }
        }
    }
    
    emit myShareCodesReceived(codes);
}

void NetworkManager::deleteShareCode(const QString &share_code) {
    if (socket->state() != QAbstractSocket::ConnectedState) {
        emit shareCodeDeleted(false, "Not connected to server!");
        return;
    }
    
    disconnect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    QString cmd = QString("%1 %2\n").arg(CMD_DELETE_SHARE_CODE).arg(share_code);
    socket->write(cmd.toUtf8());
    socket->flush();
    
    if (!socket->waitForReadyRead(5000)) {
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        emit shareCodeDeleted(false, "Timeout waiting for server response");
        return;
    }
    
    QString response = QString::fromUtf8(socket->readAll()).trimmed();
    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    if (response.startsWith(CODE_OK)) {
        emit shareCodeDeleted(true, "Share code deleted successfully!");
    } else {
        emit shareCodeDeleted(false, response);
    }
}

// ===== GUEST MODE FUNCTIONS =====

bool NetworkManager::isConnected() const {
    return socket && socket->state() == QAbstractSocket::ConnectedState;
}

void NetworkManager::guestRedeemShareCode(const QString &share_code) {
    if (socket->state() != QAbstractSocket::ConnectedState) {
        emit guestRedeemResult(false, 0, "", false, "", 0);
        return;
    }
    
    disconnect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    // Use a special guest redeem command that doesn't require login
    QString cmd = QString("GUEST_REDEEM %1\n").arg(share_code);
    socket->write(cmd.toUtf8());
    socket->flush();
    
    if (!socket->waitForReadyRead(5000)) {
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        emit guestRedeemResult(false, 0, "", false, "", 0);
        return;
    }
    
    QString response = QString::fromUtf8(socket->readAll()).trimmed();
    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    // Expected response: 200 file_id|filename|is_folder|owner|size
    if (response.startsWith(CODE_OK)) {
        QString data = response.mid(4).trimmed();
        QStringList parts = data.split('|');
        if (parts.size() >= 5) {
            long long file_id = parts[0].toLongLong();
            QString filename = parts[1];
            bool is_folder = (parts[2] == "1");
            QString owner = parts[3];
            long long size = parts[4].toLongLong();
            emit guestRedeemResult(true, file_id, filename, is_folder, owner, size);
            return;
        }
    }
    
    emit guestRedeemResult(false, 0, "", false, "", 0);
}

void NetworkManager::guestListFolder(long long folder_id) {
    if (socket->state() != QAbstractSocket::ConnectedState) {
        emit guestFolderList(QList<GuestFileInfo>());
        return;
    }
    
    disconnect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    QString cmd = QString("GUEST_LIST %1\n").arg(folder_id);
    socket->write(cmd.toUtf8());
    socket->flush();
    
    if (!socket->waitForReadyRead(5000)) {
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        emit guestFolderList(QList<GuestFileInfo>());
        return;
    }
    
    QString response = QString::fromUtf8(socket->readAll()).trimmed();
    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    QList<GuestFileInfo> files;
    
    // Expected: 200 file_id|filename|is_folder|size|owner;file_id|...
    if (response.startsWith(CODE_OK)) {
        QString data = response.mid(4).trimmed();
        QStringList items = data.split(';', Qt::SkipEmptyParts);
        
        for (const QString &item : items) {
            QStringList parts = item.split('|');
            if (parts.size() >= 5) {
                GuestFileInfo info;
                info.file_id = parts[0].toLongLong();
                info.filename = parts[1];
                info.is_folder = (parts[2] == "1");
                info.size = parts[3].toLongLong();
                info.owner = parts[4];
                files.append(info);
            }
        }
    }
    
    emit guestFolderList(files);
}