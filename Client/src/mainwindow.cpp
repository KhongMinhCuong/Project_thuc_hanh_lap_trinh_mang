#include "mainwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QLabel>
#include <QFileDialog>
#include <QInputDialog>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    netManager = new NetworkManager(this);
    setupUI();

    // Kết nối các tín hiệu logic mạng với giao diện
    connect(netManager, &NetworkManager::loginSuccess, this, &MainWindow::handleLoginSuccess);
    connect(netManager, &NetworkManager::loginFailed, this, [this](QString msg){
        QMessageBox::critical(this, "Login Error", msg);
    });
    connect(netManager, &NetworkManager::fileListReceived, this, &MainWindow::handleFileList);
    connect(netManager, &NetworkManager::connectionStatus, this, [this](bool success, QString msg){
        if(success) QMessageBox::information(this, "Network", msg);
        else QMessageBox::warning(this, "Network", msg);
    });
    connect(netManager, &NetworkManager::uploadProgress, this, &MainWindow::handleUploadProgress);
    connect(netManager, &NetworkManager::downloadComplete, this, &MainWindow::handleDownloadComplete);
    connect(netManager, &NetworkManager::shareResult, this, &MainWindow::handleShareResult);
    connect(netManager, &NetworkManager::deleteResult, this, &MainWindow::handleDeleteResult);
    connect(netManager, &NetworkManager::logoutSuccess, this, &MainWindow::handleLogout);
}

MainWindow::~MainWindow() {}

void MainWindow::setupUI() {
    stackedWidget = new QStackedWidget(this);
    stackedWidget->addWidget(createLoginPage());     // Index 0
    stackedWidget->addWidget(createDashboardPage()); // Index 1
    setCentralWidget(stackedWidget);
    resize(800, 500);
}

QWidget* MainWindow::createLoginPage() {
    QWidget *p = new QWidget;
    QVBoxLayout *l = new QVBoxLayout(p);
    
    hostInput = new QLineEdit("127.0.0.1");
    userInput = new QLineEdit; userInput->setPlaceholderText("Username");
    passInput = new QLineEdit; passInput->setPlaceholderText("Password");
    passInput->setEchoMode(QLineEdit::Password);
    
    QPushButton *btnConnect = new QPushButton("1. Connect Server");
    QPushButton *btnLogin = new QPushButton("2. Login");

    l->addWidget(new QLabel("Server IP:"));
    l->addWidget(hostInput);
    l->addWidget(btnConnect);
    l->addSpacing(20);
    l->addWidget(userInput);
    l->addWidget(passInput);
    l->addWidget(btnLogin);
    l->addStretch();

    connect(btnConnect, &QPushButton::clicked, this, &MainWindow::onConnectBtnClicked);
    connect(btnLogin, &QPushButton::clicked, this, &MainWindow::onLoginBtnClicked);
    return p;
}

QWidget* MainWindow::createDashboardPage() {
    QWidget *p = new QWidget;
    QVBoxLayout *l = new QVBoxLayout(p);

    // Top bar với nút Logout
    QHBoxLayout *topBar = new QHBoxLayout;
    QLabel *welcomeLabel = new QLabel("File Management Dashboard");
    QPushButton *btnLogout = new QPushButton("Logout");
    btnLogout->setMaximumWidth(100);
    topBar->addWidget(welcomeLabel);
    topBar->addStretch();
    topBar->addWidget(btnLogout);

    // Buttons row
    QHBoxLayout *btnLayout = new QHBoxLayout;
    QPushButton *btnRefresh = new QPushButton("Refresh");
    QPushButton *btnUpload = new QPushButton("Upload File");
    QPushButton *btnDownload = new QPushButton("Download Selected");
    QPushButton *btnShare = new QPushButton("Share File");
    QPushButton *btnDelete = new QPushButton("Delete File");
    
    btnLayout->addWidget(btnRefresh);
    btnLayout->addWidget(btnUpload);
    btnLayout->addWidget(btnDownload);
    btnLayout->addWidget(btnShare);
    btnLayout->addWidget(btnDelete);
    btnLayout->addStretch();
    
    // Tạo TabWidget với 2 tabs
    tabWidget = new QTabWidget;
    
    // Tab 1: My Files
    fileTable = new QTableWidget;
    fileTable->setColumnCount(3);
    fileTable->setHorizontalHeaderLabels({"Filename", "Size", "Owner"});
    fileTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    fileTable->setSelectionBehavior(QTableWidget::SelectRows);
    
    // Tab 2: Shared Files
    sharedFileTable = new QTableWidget;
    sharedFileTable->setColumnCount(3);
    sharedFileTable->setHorizontalHeaderLabels({"Filename", "Size", "Shared By"});
    sharedFileTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    sharedFileTable->setSelectionBehavior(QTableWidget::SelectRows);
    
    tabWidget->addTab(fileTable, "My Files");
    tabWidget->addTab(sharedFileTable, "Shared with Me");

    l->addLayout(topBar);
    l->addLayout(btnLayout);
    l->addWidget(tabWidget);

    connect(btnRefresh, &QPushButton::clicked, this, &MainWindow::onRefreshClicked);
    connect(btnUpload, &QPushButton::clicked, this, &MainWindow::onUploadClicked);
    connect(btnDownload, &QPushButton::clicked, this, &MainWindow::onDownloadClicked);
    connect(btnShare, &QPushButton::clicked, this, &MainWindow::onShareClicked);
    connect(btnDelete, &QPushButton::clicked, this, &MainWindow::onDeleteClicked);
    connect(btnLogout, &QPushButton::clicked, this, &MainWindow::onLogoutClicked);
    connect(tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);
    return p;
}

// --- Logic UI ---

void MainWindow::onConnectBtnClicked() {
    netManager->connectToServer(hostInput->text(), 8080);
}

void MainWindow::onLoginBtnClicked() {
    netManager->login(userInput->text(), passInput->text());
}

void MainWindow::handleLoginSuccess() {
    stackedWidget->setCurrentIndex(1); // Chuyển sang trang Dashboard
    netManager->requestFileList();     // Tự động lấy danh sách file
}

void MainWindow::onRefreshClicked() {
    // Refresh tab hiện tại
    onTabChanged(tabWidget->currentIndex());
}

void MainWindow::onTabChanged(int index) {
    if (index == 0) {
        // Tab "My Files" - request LIST
        netManager->requestFileList();
    } else if (index == 1) {
        // Tab "Shared with Me" - request LISTSHARED
        netManager->requestSharedFileList();
    }
}

void MainWindow::handleFileList(QString data) {
    // Xác định table nào cần update dựa vào tab hiện tại
    QTableWidget* targetTable = (tabWidget->currentIndex() == 0) ? fileTable : sharedFileTable;
    
    targetTable->setRowCount(0); // Xóa cũ
    // Giả sử server gửi: "file1.txt|1024|admin\nfile2.jpg|2048|user"
    QStringList rows = data.split('\n', Qt::SkipEmptyParts);
    
    for(const QString &line : rows) {
        QStringList cols = line.split('|');
        if(cols.size() >= 3) {
            int row = targetTable->rowCount();
            targetTable->insertRow(row);
            targetTable->setItem(row, 0, new QTableWidgetItem(cols[0]));
            targetTable->setItem(row, 1, new QTableWidgetItem(cols[1]));
            targetTable->setItem(row, 2, new QTableWidgetItem(cols[2]));
        }
    }
}

void MainWindow::onUploadClicked() {
    QString filePath = QFileDialog::getOpenFileName(this, "Select File to Upload");
    if (!filePath.isEmpty()) {
        netManager->uploadFile(filePath);
    }
}

void MainWindow::onDownloadClicked() {
    // Lấy table hiện tại dựa vào tab
    QTableWidget* currentTable = (tabWidget->currentIndex() == 0) ? fileTable : sharedFileTable;
    
    int row = currentTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Download", "Please select a file first!");
        return;
    }
    
    QString filename = currentTable->item(row, 0)->text();
    
    // Cho phép người dùng chọn nơi lưu file
    QString savePath = QFileDialog::getSaveFileName(this, 
                                                     "Save File As",
                                                     QDir::homePath() + "/Downloads/" + filename,
                                                     "All Files (*)");
    
    if (!savePath.isEmpty()) {
        netManager->downloadFile(filename, savePath);
    }
}

void MainWindow::handleUploadProgress(QString msg) {
    QMessageBox::information(this, "Upload", msg);
    // Không cần gọi refresh ở đây nữa vì NetworkManager đã tự refresh
}

void MainWindow::handleDownloadComplete(QString filename) {
    QMessageBox::information(this, "Download", "File saved: " + filename);
}

void MainWindow::onShareClicked() {
    // Chỉ share được file trong "My Files"
    if (tabWidget->currentIndex() != 0) {
        QMessageBox::warning(this, "Share", "You can only share files from 'My Files' tab!");
        return;
    }
    
    int row = fileTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Share", "Please select a file first!");
        return;
    }
    
    QString filename = fileTable->item(row, 0)->text();
    
    // Hiển thị dialog nhập username
    bool ok;
    QString targetUser = QInputDialog::getText(this, "Share File",
                                               "Share \"" + filename + "\" to username:",
                                               QLineEdit::Normal, "", &ok);
    if (ok && !targetUser.isEmpty()) {
        netManager->shareFile(filename, targetUser);
    }
}

void MainWindow::handleShareResult(bool success, QString msg) {
    if (success) {
        QMessageBox::information(this, "Share", msg);
    } else {
        QMessageBox::warning(this, "Share", msg);
    }
}

void MainWindow::onDeleteClicked() {
    // Chỉ xóa được file trong "My Files"
    if (tabWidget->currentIndex() != 0) {
        QMessageBox::warning(this, "Delete", "You can only delete files from 'My Files' tab!");
        return;
    }
    
    int row = fileTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Delete", "Please select a file first!");
        return;
    }
    
    QString filename = fileTable->item(row, 0)->text();
    
    // Xác nhận xóa
    auto reply = QMessageBox::question(this, "Delete File",
                                       "Are you sure you want to delete \"" + filename + "\"?",
                                       QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        netManager->deleteFile(filename);
    }
}

void MainWindow::handleDeleteResult(bool success, QString msg) {
    if (success) {
        QMessageBox::information(this, "Delete", msg);
        // Refresh danh sách sau khi xóa
        netManager->requestFileList();
    } else {
        QMessageBox::warning(this, "Delete", msg);
    }
}

void MainWindow::onLogoutClicked() {
    auto reply = QMessageBox::question(this, "Logout",
                                       "Are you sure you want to logout?",
                                       QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        netManager->logout();
    }
}

void MainWindow::handleLogout() {
    // Xóa dữ liệu cả 2 bảng
    fileTable->setRowCount(0);
    sharedFileTable->setRowCount(0);
    
    // Chuyển về tab đầu tiên
    tabWidget->setCurrentIndex(0);
    
    // Chuyển về trang Login
    stackedWidget->setCurrentIndex(0);
    
    // Xóa mật khẩu (tùy chọn bảo mật)
    passInput->clear();
    
    QMessageBox::information(this, "Logout", "You have been logged out successfully.");
}