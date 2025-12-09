#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <QObject>
#include <QTcpSocket>
#include "../../Common/Protocol.h"

class NetworkManager : public QObject {
    Q_OBJECT
public:
    explicit NetworkManager(QObject *parent = nullptr);
    void connectToServer(const QString &host, quint16 port);
    void login(const QString &username, const QString &password);
    void requestFileList(); // Gửi lệnh LIST (my files)
    void requestSharedFileList(); // Gửi lệnh LISTSHARED (shared files)
    void uploadFile(const QString &filePath); // Upload file
    void downloadFile(const QString &filename, const QString &savePath); // Download file
    void shareFile(const QString &filename, const QString &targetUser); // Share file
    void deleteFile(const QString &filename); // Delete file
    void logout(); // Đăng xuất

signals:
    void connectionStatus(bool success, QString message);
    void loginSuccess();
    void loginFailed(QString reason);
    void fileListReceived(QString rawData); // Nhận dữ liệu danh sách file
    void uploadProgress(QString message);
    void downloadComplete(QString filename);
    void shareResult(bool success, QString message);
    void deleteResult(bool success, QString message);
    void logoutSuccess();

private slots:
    void onReadyRead(); // Xử lý dữ liệu Server gửi về

private:
    QTcpSocket *socket;
    
    // Lưu thông tin để reconnect
    QString currentHost;
    quint16 currentPort;
    QString currentUsername;
    QString currentPassword;
};

#endif // NETWORK_MANAGER_H