#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <QObject>
#include <QTcpSocket>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>

#define CMD_USER "USER"
#define CMD_PASS "PASS"
#define CMD_REGISTER "REGISTER"
#define CMD_LIST "LIST"
#define CMD_LISTSHARED "LISTSHARED"
#define CMD_UPLOAD_CHECK "SITE QUOTA_CHECK"
#define CMD_UPLOAD "STOR"
#define CMD_DOWNLOAD "RETR"
#define CMD_SHARE "SHARE"
#define CMD_DELETE "DELETE"
#define CMD_RENAME "RENAME"

#define CMD_GET_FOLDER_STRUCTURE "GET_FOLDER_STRUCTURE"
#define CMD_SHARE_FOLDER "SHARE_FOLDER"
#define CMD_UPLOAD_FILE "UPLOAD_FILE"
#define CMD_DOWNLOAD_FOLDER "DOWNLOAD_FOLDER"
#define CMD_CHECK_SHARE_PROGRESS "CHECK_SHARE_PROGRESS"
#define CMD_CANCEL_FOLDER_SHARE "CANCEL_FOLDER_SHARE"
#define CMD_CREATE_FOLDER "CREATE_FOLDER"

#define TYPE_FILE 1
#define TYPE_DIR  2
#define TYPE_END  3

#define CODE_OK "200"
#define CODE_FAIL "500"
#define CODE_LOGIN_SUCCESS "230"
#define CODE_LOGIN_FAIL "530"
#define CODE_DATA_OPEN "150"
#define CODE_CHUNK_ACK "151"
#define CODE_TRANSFER_COMPLETE "226"

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
    
    void connectToServer(const QString &host, quint16 port);
    void login(const QString &user, const QString &pass);
    void registerAccount(const QString &user, const QString &pass);
    void requestFileList(long long parent_id = 0);
    void requestSharedFileList(long long parent_id = -1); // -1 = root, >= 0 = specific folder
    void uploadFile(const QString &filePath, long long parent_id = 0);
    void uploadFolder(const QString &folderPath, long long parent_id = 0);
    void downloadFile(const QString &filename, const QString &savePath);
    void downloadFolder(const QString &foldername, const QString &savePath);
    void shareFile(const QString &filename, const QString &targetUser);
    void deleteFile(const QString &filename);
    void renameItem(const QString &fileId, const QString &newName, const QString &itemType);
    void logout();

    void getFolderStructure(long long folder_id);
    void shareFolderRequest(long long folder_id, const QString &targetUser);
    void uploadFolderFile(const QString &session_id, const FileNodeInfo &fileInfo, const QString &localBasePath);
    void checkShareProgress(const QString &session_id);
    void cancelFolderShare(const QString &session_id);

signals:
    void connectionStatus(bool success, QString msg);
    void loginSuccess();
    void loginFailed(QString msg);
    void registerSuccess(QString msg);
    void registerFailed(QString msg);
    void fileListReceived(QString data);
    void uploadStarted(QString filename);
    void uploadProgress(QString msg);
    void downloadStarted(QString filename);
    void downloadComplete(QString filename);
    void shareResult(bool success, QString msg);
    void deleteResult(bool success, QString msg);
    void renameResult(bool success, QString msg);
    void logoutSuccess();
    void transferProgress(qint64 current, qint64 total);

    void folderStructureReceived(long long folder_id, QList<FileNodeInfo> structure);
    void folderShareInitiated(const QString &session_id, int total_files, const QList<FileNodeInfo> &files);
    void folderFileUploaded(int completed, int total);
    void folderShareCompleted(const QString &session_id);
    void folderShareFailed(const QString &error);
    void folderShareProgress(int percentage, const QString &status);
    void folderUploadStarted(const QString &folderName, int totalFiles);
    void folderUploadProgress(int currentFile, int totalFiles, const QString &currentFileName);
    void folderUploadCompleted(const QString &folderName);

private slots:
    void onReadyRead();

private:
    bool ensureConnected();
    
    QTcpSocket *socket;
    QString currentHost;
    quint16 currentPort;
    QString currentUsername;
    QString currentPassword;
    long long currentParentId = 0;
    
    FolderShareSessionInfo currentFolderShare;
    bool isFolderShareActive = false;
};

#endif