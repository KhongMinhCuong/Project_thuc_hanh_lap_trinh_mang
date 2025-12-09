#include "network_manager.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QTimer>

NetworkManager::NetworkManager(QObject *parent) : QObject(parent) {
    socket = new QTcpSocket(this);
    
    // Kết nối signal đọc dữ liệu
    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
}

void NetworkManager::connectToServer(const QString &host, quint16 port) {
    // Lưu thông tin kết nối
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
    if(socket->state() != QAbstractSocket::ConnectedState) return;

    // Gửi: USER <username>
    QString cmdUser = QString("%1 %2\n").arg(CMD_USER, user);
    socket->write(cmdUser.toUtf8());
    socket->flush();

    // Gửi: PASS <password> (Thực tế nên đợi server phản hồi 331 rồi mới gửi PASS)
    socket->waitForBytesWritten(100); 
    QString cmdPass = QString("%1 %2\n").arg(CMD_PASS, pass);
    socket->write(cmdPass.toUtf8());
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

    // Tạm thời disconnect readyRead để tự xử lý response
    disconnect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);

    // 1. Kiểm tra quota
    QString checkCmd = QString("%1 %2 %3\n").arg(CMD_UPLOAD_CHECK, filename).arg(filesize);
    qDebug() << "[Upload] Sending quota check:" << checkCmd.trimmed();
    socket->write(checkCmd.toUtf8());
    socket->flush();
    
    if (!socket->waitForReadyRead(5000)) {
        emit uploadProgress("Quota check timeout!");
        file.close();
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        return;
    }
    
    QString response = QString::fromUtf8(socket->readAll()).trimmed();
    qDebug() << "[Upload] Quota check response:" << response;
    
    if (!response.startsWith(CODE_OK)) {
        emit uploadProgress("Quota check failed: " + response);
        file.close();
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        return;
    }

    // 2. Gửi lệnh STOR
    QString storCmd = QString("%1 %2 %3\n").arg(CMD_UPLOAD, filename).arg(filesize);
    qDebug() << "[Upload] Sending STOR command:" << storCmd.trimmed();
    socket->write(storCmd.toUtf8());
    socket->flush();
    
    if (!socket->waitForReadyRead(5000)) {
        emit uploadProgress("STOR timeout!");
        file.close();
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        return;
    }
    
    response = QString::fromUtf8(socket->readAll()).trimmed();
    qDebug() << "[Upload] STOR response:" << response;
    
    if (!response.startsWith(CODE_DATA_OPEN)) {
        emit uploadProgress("Upload rejected: " + response);
        file.close();
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        return;
    }

    // 3. Gửi dữ liệu file
    qDebug() << "[Upload] Starting file transfer...";
    qint64 totalSent = 0;
    char buffer[4096];
    while (!file.atEnd()) {
        qint64 bytesRead = file.read(buffer, sizeof(buffer));
        socket->write(buffer, bytesRead);
        totalSent += bytesRead;
    }
    socket->flush();
    file.close();
    
    qDebug() << "[Upload] File data sent:" << totalSent << "bytes";

    // 4. Đợi server xác nhận
    if (!socket->waitForReadyRead(10000)) {
        emit uploadProgress("Transfer complete timeout!");
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        return;
    }
    
    response = QString::fromUtf8(socket->readAll()).trimmed();
    qDebug() << "[Upload] Final response:" << response;
    
    // Kết nối lại readyRead slot
    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    if (response.startsWith(CODE_TRANSFER_COMPLETE)) {
        emit uploadProgress("Upload successful: " + filename);
        
        // Server KHÔNG đóng socket nữa - socket được trả về WorkerThread
        // Chỉ cần request file list sau 200ms
        QTimer::singleShot(200, this, [this]() {
            qDebug() << "[Upload] Requesting updated file list...";
            requestFileList();
        });
    } else {
        emit uploadProgress("Upload completed but server response: " + response);
    }
}

void NetworkManager::downloadFile(const QString &filename, const QString &savePath) {
    if(socket->state() != QAbstractSocket::ConnectedState) {
        emit downloadComplete("Not connected to server!");
        return;
    }

    qDebug() << "[Download] Downloading" << filename << "to" << savePath;
    
    // Tạm disconnect readyRead
    disconnect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    // 1. Gửi lệnh RETR
    QString cmd = QString("%1 %2\n").arg(CMD_DOWNLOAD, filename);
    socket->write(cmd.toUtf8());
    socket->flush();
    
    if (!socket->waitForReadyRead(5000)) {
        emit downloadComplete("Download timeout waiting for response!");
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        return;
    }

    QString response = QString::fromUtf8(socket->readLine()).trimmed();
    qDebug() << "[Download] Response:" << response;
    
    // Kiểm tra nếu bị từ chối
    if (response.startsWith(CODE_FAIL)) {
        emit downloadComplete("Download failed: " + response);
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        return;
    }

    if (!response.startsWith(CODE_DATA_OPEN)) {
        emit downloadComplete("Unexpected response: " + response);
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        return;
    }

    // Lấy filesize từ response "150 <filesize>"
    QStringList parts = response.split(' ');
    qint64 filesize = 0;
    if (parts.size() >= 2) {
        filesize = parts[1].toLongLong();
    }
    qDebug() << "[Download] File size:" << filesize;

    // 2. Chuẩn bị lưu file
    QFile file(savePath);
    if (!file.open(QIODevice::WriteOnly)) {
        emit downloadComplete("Cannot save file to: " + savePath);
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        return;
    }

    // 3. Nhận dữ liệu file
    qint64 totalReceived = 0;
    QByteArray fileData;
    
    while (totalReceived < filesize) {
        if (!socket->waitForReadyRead(10000)) {
            qDebug() << "[Download] Timeout waiting for data";
            break;
        }
        
        QByteArray chunk = socket->readAll();
        fileData.append(chunk);
        totalReceived = fileData.size();
        
        qDebug() << "[Download] Progress:" << totalReceived << "/" << filesize;
    }
    
    // Ghi dữ liệu vào file
    file.write(fileData);
    file.close();
    
    qDebug() << "[Download] Received" << totalReceived << "bytes";

    // 4. Đợi thông báo hoàn tất (226)
    if (socket->waitForReadyRead(2000)) {
        QString finalMsg = QString::fromUtf8(socket->readLine()).trimmed();
        qDebug() << "[Download] Final message:" << finalMsg;
    }
    
    // Kết nối lại readyRead
    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    emit downloadComplete("File saved to: " + savePath);
}

void NetworkManager::shareFile(const QString &filename, const QString &targetUser) {
    if(socket->state() != QAbstractSocket::ConnectedState) {
        emit shareResult(false, "Not connected to server!");
        return;
    }

    qDebug() << "[Share] Sharing" << filename << "to" << targetUser;
    
    // Gửi lệnh SHARE <filename> <target_username>
    QString cmd = QString("SHARE %1 %2\n").arg(filename, targetUser);
    socket->write(cmd.toUtf8());
    socket->flush();
    
    // Tạm disconnect để đợi response
    disconnect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    if (!socket->waitForReadyRead(5000)) {
        emit shareResult(false, "Share timeout!");
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        return;
    }
    
    QString response = QString::fromUtf8(socket->readAll()).trimmed();
    qDebug() << "[Share] Response:" << response;
    
    // Kết nối lại readyRead
    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    if (response.startsWith(CODE_OK)) {
        emit shareResult(true, "File shared successfully!");
    } else {
        emit shareResult(false, "Share failed: " + response);
    }
}

void NetworkManager::deleteFile(const QString &filename) {
    if(socket->state() != QAbstractSocket::ConnectedState) {
        emit deleteResult(false, "Not connected to server!");
        return;
    }

    qDebug() << "[Delete] Deleting" << filename;
    
    // Gửi lệnh DELETE <filename>
    QString cmd = QString("DELETE %1\n").arg(filename);
    socket->write(cmd.toUtf8());
    socket->flush();
    
    // Tạm disconnect để đợi response
    disconnect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    if (!socket->waitForReadyRead(5000)) {
        emit deleteResult(false, "Delete timeout!");
        connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
        return;
    }
    
    QString response = QString::fromUtf8(socket->readAll()).trimmed();
    qDebug() << "[Delete] Response:" << response;
    
    // Kết nối lại readyRead
    connect(socket, &QTcpSocket::readyRead, this, &NetworkManager::onReadyRead);
    
    if (response.startsWith(CODE_OK)) {
        emit deleteResult(true, "File deleted successfully!");
    } else {
        emit deleteResult(false, "Delete failed: " + response);
    }
}

void NetworkManager::logout() {
    qDebug() << "[Logout] Disconnecting from server...";
    
    // Ngắt kết nối
    if (socket->state() == QAbstractSocket::ConnectedState) {
        socket->disconnectFromHost();
        if (socket->state() != QAbstractSocket::UnconnectedState) {
            socket->waitForDisconnected(1000);
        }
    }
    
    // Xóa thông tin đăng nhập
    currentUsername.clear();
    currentPassword.clear();
    
    qDebug() << "[Logout] Logged out successfully";
    emit logoutSuccess();
}

void NetworkManager::onReadyRead() {
    QByteArray data = socket->readAll();
    QString response = QString::fromUtf8(data).trimmed();

    qDebug() << "[Network] Received:" << response.left(100); // Log first 100 chars

    // Phân tích phản hồi đơn giản (Dựa trên Protocol)
    if (response.startsWith(CODE_LOGIN_SUCCESS)) {
        emit loginSuccess();
    } 
    else if (response.startsWith(CODE_LOGIN_FAIL)) {
        emit loginFailed("Invalid Username or Password");
    }
    // QUAN TRỌNG: Chỉ emit file list nếu có dấu | (định dạng file list)
    // Tránh trường hợp response code (200, 226, 331...) bị coi là file list
    else if (response.contains("|")) { 
        emit fileListReceived(response);
    }
    // Các response khác (200, 226, 331...) không làm gì
}