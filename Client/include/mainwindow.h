#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTabWidget>
#include "network_manager.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Login page
    void onConnectBtnClicked();
    void onLoginBtnClicked();
    void handleLoginSuccess();

    // Dashboard page
    void onRefreshClicked();
    void onCreateFolderClicked();      // NEW
    void onUploadClicked();
    void onDownloadClicked();
    void onShareClicked();
    void onDeleteClicked();
    void onLogoutClicked();
    void onTabChanged(int index);
    
    // Folder share
    void onShareFolderClicked();
    void showContextMenu(const QPoint &pos);

    // Network responses
    void handleFileList(QString data);
    void handleUploadProgress(QString msg);
    void handleDownloadComplete(QString filename);
    void handleShareResult(bool success, QString msg);
    void handleDeleteResult(bool success, QString msg);
    void handleFolderCreated(bool success, QString message, long long folder_id);  // NEW
    void handleLogout();

private:
    void setupUI();
    QWidget* createLoginPage();
    QWidget* createDashboardPage();

    // UI Components
    QStackedWidget *stackedWidget;
    
    // Login page
    QLineEdit *hostInput;
    QLineEdit *userInput;
    QLineEdit *passInput;
    
    // Dashboard page
    QTabWidget *tabWidget;
    QTableWidget *fileTable;
    QTableWidget *sharedFileTable;
    
    // Network manager
    NetworkManager *netManager;
};

#endif // MAINWINDOW_H