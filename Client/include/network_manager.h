#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <QObject>
#include <QTcpSocket>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>

// Protocol Commands
#define CMD_USER "USER"
#define CMD_PASS "PASS"
#define CMD_LIST "LIST"
#define CMD_LISTSHARED "LISTSHARED"
#define CMD_UPLOAD_CHECK "SITE QUOTA_CHECK"
#define CMD_UPLOAD "STOR"
#define CMD_DOWNLOAD "RETR"
#define CMD_SHARE "SHARE"
#define CMD_DELETE "DELETE"
#define CMD_CREATE_FOLDER "MKDIR"  // NEW

// Folder share commands
#define CMD_GET_FOLDER_STRUCTURE "GET_FOLDER_STRUCTURE"
#define CMD_SHARE_FOLDER "SHARE_FOLDER"
#define CMD_UPLOAD_FOLDER_FILE "UPLOAD_FOLDER_FILE"
#define CMD_CHECK_SHARE_PROGRESS "CHECK_SHARE_PROGRESS"
#define CMD_CANCEL_FOLDER_SHARE "CANCEL_FOLDER_SHARE"

// Response Codes
#define CODE_OK "200"
#define CODE_FAIL "500"
#define CODE_LOGIN_SUCCESS "230"
#define CODE_LOGIN_FAIL "530"
#define CODE_DATA_OPEN "150"
#define CODE_TRANSFER_COMPLETE "226"

// Folder share structures
struct FileNodeInfo {
    long long file_id;
    QString name;
    bool is_folder;
    long long size;
    long long parent_id;
    QString relative_path;
};

struct FolderShareSessionInfo {
    QString session_id;
    long long folder_id;
    int total_files;
    int completed_files;
    QList<FileNodeInfo> files_to_upload;
};

class NetworkManager : public QObject {
    Q_OBJECT

public:
    explicit NetworkManager(QObject *parent = nullptr);
    
    // Existing functions
    void connectToServer(const QString &host, quint16 port);
    void login(const QString &user, const QString &pass);
    void requestFileList();
    void requestSharedFileList();
    void uploadFile(const QString &filePath);
    void downloadFile(const QString &filename, const QString &savePath);
    void shareFile(const QString &filename, const QString &targetUser);
    void deleteFile(const QString &filename);
    void logout();

    // NEW: Create folder
    void createFolder(const QString &folderName, long long parent_id = 1);

    // Folder share functions
    void getFolderStructure(long long folder_id);
    void shareFolderRequest(long long folder_id, const QString &targetUser);
    void uploadFolderFile(const QString &session_id, const FileNodeInfo &fileInfo, const QString &localBasePath);
    void checkShareProgress(const QString &session_id);
    void cancelFolderShare(const QString &session_id);

signals:
    // Existing signals
    void connectionStatus(bool success, QString msg);
    void loginSuccess();
    void loginFailed(QString msg);
    void fileListReceived(QString data);
    void uploadProgress(QString msg);
    void downloadComplete(QString filename);
    void shareResult(bool success, QString msg);
    void deleteResult(bool success, QString msg);
    void logoutSuccess();
    void transferProgress(qint64 current, qint64 total);

    // NEW: Folder creation
    void folderCreated(bool success, QString message, long long folder_id);

    // Folder share signals
    void folderStructureReceived(long long folder_id, QList<FileNodeInfo> structure);
    void folderShareInitiated(const QString &session_id, int total_files, const QList<FileNodeInfo> &files);
    void folderFileUploaded(int completed, int total);
    void folderShareCompleted(const QString &session_id);
    void folderShareFailed(const QString &error);
    void folderShareProgress(int percentage, const QString &status);

private slots:
    void onReadyRead();

private:
    QTcpSocket *socket;
    QString currentHost;
    quint16 currentPort;
    QString currentUsername;
    QString currentPassword;
    
    // Folder share state
    FolderShareSessionInfo currentFolderShare;
    bool isFolderShareActive = false;
};

#endif // NETWORK_MANAGER_H