#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTabWidget>
#include <QLabel>
#include <QProgressDialog>
#include <QHBoxLayout>
#include <QStack>
#include "network_manager.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onConnectBtnClicked();
    void onLoginBtnClicked();
    void onRegisterBtnClicked();
    void handleLoginSuccess();
    void handleRegisterSuccess(QString msg);
    void handleRegisterFailed(QString msg);

    void onRefreshClicked();
    void onUploadClicked();
    void onUploadFileClicked();
    void onUploadFolderClicked();
    void onDownloadClicked();
    void onDownloadFolderClicked();
    void onShareClicked();
    void onDeleteClicked();
    void onRenameClicked();
    void onLogoutClicked();
    void onTabChanged(int index);
    
    void showContextMenu(const QPoint &pos);
    void onFolderDoubleClicked(int row, int column);
    void onBackButtonClicked();
    void onBreadcrumbClicked();

    void handleFileList(QString data);
    void handleUploadStarted(QString filename);
    void handleUploadProgress(QString msg);
    void handleDownloadStarted(QString filename);
    void handleDownloadComplete(QString filename);
    void handleShareResult(bool success, QString msg);
    void handleDeleteResult(bool success, QString msg);
    void handleRenameResult(bool success, QString msg);
    void handleLogout();

private:
    void setupUI();
    QWidget* createLoginPage();
    QWidget* createDashboardPage();

    QStackedWidget *stackedWidget;
    
    QLineEdit *hostInput;
    QLineEdit *userInput;
    QLineEdit *passInput;
    
    QTabWidget *tabWidget;
    QTableWidget *fileTable;
    QTableWidget *sharedFileTable;
    QPushButton *backButton;
    QLabel *pathLabel;
    QHBoxLayout *breadcrumbLayout;
    
    long long currentFolderId;
    QString currentFolderPath;
    QString currentUsername;
    
    QStack<long long> folderHistory;
    
    NetworkManager *netManager;
    
    QProgressDialog *uploadProgressDialog;
    QProgressDialog *downloadProgressDialog;
};

#endif