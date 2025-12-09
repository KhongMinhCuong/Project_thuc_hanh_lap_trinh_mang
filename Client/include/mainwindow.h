#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QLineEdit>
#include <QTableWidget>
#include <QTabWidget>
#include <QPushButton>
#include "network_manager.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onConnectBtnClicked();
    void onLoginBtnClicked();
    void onRefreshClicked(); // Nút làm mới danh sách
    void onUploadClicked();  // Nút upload file
    void onDownloadClicked(); // Nút download file
    void onShareClicked();   // Nút share file
    void onDeleteClicked();  // Nút xóa file
    void onLogoutClicked();  // Nút đăng xuất
    void onTabChanged(int index);  // Chuyển tab
    
    // Slots nhận tín hiệu từ NetworkManager
    void handleLoginSuccess();
    void handleFileList(QString data);
    void handleUploadProgress(QString msg);
    void handleDownloadComplete(QString filename);
    void handleShareResult(bool success, QString msg);
    void handleDeleteResult(bool success, QString msg);
    void handleLogout();

private:
    void setupUI();
    QWidget* createLoginPage();
    QWidget* createDashboardPage();

    NetworkManager *netManager;
    QStackedWidget *stackedWidget;
    
    // UI Elements Login
    QLineEdit *hostInput;
    QLineEdit *userInput;
    QLineEdit *passInput;
    
    // UI Elements Dashboard
    QTabWidget *tabWidget;
    QTableWidget *fileTable;        // My Files
    QTableWidget *sharedFileTable;  // Shared Files
};

#endif // MAINWINDOW_H