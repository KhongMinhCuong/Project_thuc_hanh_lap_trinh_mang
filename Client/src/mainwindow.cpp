#include "mainwindow.h"
#include "FolderShareDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QLabel>
#include <QFileDialog>
#include <QInputDialog>
#include <QMenu>

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
    
    // NEW: Folder creation signal
    connect(netManager, &NetworkManager::folderCreated, this, &MainWindow::handleFolderCreated);
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
    QPushButton *btnCreateFolder = new QPushButton("Create Folder");
    QPushButton *btnUpload = new QPushButton("Upload File");
    QPushButton *btnDownload = new QPushButton("Download Selected");
    QPushButton *btnShare = new QPushButton("Share File");
    QPushButton *btnDelete = new QPushButton("Delete File");
    QPushButton *btnShareFolder = new QPushButton("Share Folder");
    
    btnLayout->addWidget(btnRefresh);
    btnLayout->addWidget(btnCreateFolder);
    btnLayout->addWidget(btnUpload);
    btnLayout->addWidget(btnDownload);
    btnLayout->addWidget(btnShare);
    btnLayout->addWidget(btnDelete);
    btnLayout->addWidget(btnShareFolder);
    btnLayout->addStretch();
    
    // Tạo TabWidget với 2 tabs
    tabWidget = new QTabWidget;
    
    // Tab 1: My Files
    fileTable = new QTableWidget;
    fileTable->setColumnCount(3);
    fileTable->setHorizontalHeaderLabels({"Filename", "Size", "Owner"});
    fileTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    fileTable->setSelectionBehavior(QTableWidget::SelectRows);
    
    // Enable context menu cho fileTable
    fileTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(fileTable, &QTableWidget::customContextMenuRequested, 
            this, &MainWindow::showContextMenu);
    
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
    connect(btnCreateFolder, &QPushButton::clicked, this, &MainWindow::onCreateFolderClicked);
    connect(btnUpload, &QPushButton::clicked, this, &MainWindow::onUploadClicked);
    connect(btnDownload, &QPushButton::clicked, this, &MainWindow::onDownloadClicked);
    connect(btnShare, &QPushButton::clicked, this, &MainWindow::onShareClicked);
    connect(btnDelete, &QPushButton::clicked, this, &MainWindow::onDeleteClicked);
    connect(btnShareFolder, &QPushButton::clicked, this, &MainWindow::onShareFolderClicked);
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
    qDebug() << "[MainWindow] Tab changed to:" << index;
    
    if (index == 0) {
        qDebug() << "[MainWindow] Requesting My Files list";
        netManager->requestFileList();
    } else if (index == 1) {
        qDebug() << "[MainWindow] Requesting Shared Files list";
        netManager->requestSharedFileList();
    }
}

void MainWindow::handleFileList(QString data) {
    // Debug: In ra data nhận được
    qDebug() << "[MainWindow] handleFileList called";
    qDebug() << "[MainWindow] Raw data length:" << data.length();
    qDebug() << "[MainWindow] Raw data:" << data;
    
    // Xác định table nào cần update
    QTableWidget* targetTable = (tabWidget->currentIndex() == 0) ? fileTable : sharedFileTable;
    
    // Xóa dữ liệu cũ
    targetTable->setRowCount(0);
    
    // Check empty
    if (data.isEmpty()) {
        qDebug() << "[MainWindow] Empty file list received";
        return;
    }
    
    // Split thành các dòng
    QStringList rows = data.split('\n', Qt::SkipEmptyParts);
    qDebug() << "[MainWindow] Number of rows after split:" << rows.size();
    
    // Parse từng dòng
    int successCount = 0;
    for (int i = 0; i < rows.size(); i++) {
        const QString &line = rows[i].trimmed();
        
        qDebug() << "[MainWindow] Processing row" << i << ":" << line;
        
        // Skip empty lines
        if (line.isEmpty()) {
            qDebug() << "[MainWindow] Skipping empty line";
            continue;
        }
        
        // Split by |
        QStringList cols = line.split('|');
        qDebug() << "[MainWindow] Columns found:" << cols.size();
        
        if (cols.size() >= 3) {
            int row = targetTable->rowCount();
            targetTable->insertRow(row);
            
            // Set items
            targetTable->setItem(row, 0, new QTableWidgetItem(cols[0].trimmed())); // Filename
            targetTable->setItem(row, 1, new QTableWidgetItem(cols[1].trimmed())); // Size
            targetTable->setItem(row, 2, new QTableWidgetItem(cols[2].trimmed())); // Owner
            
            successCount++;
            qDebug() << "[MainWindow] Added row" << row << ":" 
                     << cols[0] << "|" << cols[1] << "|" << cols[2];
        } else {
            qDebug() << "[MainWindow] WARNING: Row has insufficient columns:" << line;
            qDebug() << "[MainWindow] Columns:" << cols;
        }
    }
    
    qDebug() << "[MainWindow] Successfully added" << successCount << "files to table";
    qDebug() << "[MainWindow] Final table row count:" << targetTable->rowCount();
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

// ===== CREATE FOLDER IMPLEMENTATION =====

void MainWindow::onCreateFolderClicked() {
    // Chỉ tạo folder trong "My Files" tab
    if (tabWidget->currentIndex() != 0) {
        QMessageBox::warning(this, "Create Folder", 
            "You can only create folders in 'My Files' tab!");
        return;
    }
    
    // Input dialog để nhập tên folder
    bool ok;
    QString folderName = QInputDialog::getText(this, "Create Folder",
                                               "Enter folder name:",
                                               QLineEdit::Normal,
                                               "New Folder", &ok);
    
    if (!ok || folderName.isEmpty()) {
        return;
    }
    
    // Validate folder name
    if (folderName.contains('/') || folderName.contains('\\') || 
        folderName.contains('\0')) {
        QMessageBox::warning(this, "Create Folder", 
            "Folder name cannot contain /, \\, or null characters!");
        return;
    }
    
    // Trim whitespace
    folderName = folderName.trimmed();
    
    if (folderName.isEmpty()) {
        QMessageBox::warning(this, "Create Folder", "Folder name cannot be empty!");
        return;
    }
    
    // TODO: Nếu có chọn parent folder từ UI, lấy parent_id
    // Hiện tại default = 1 (root)
    long long parent_id = 1;
    
    // Create folder
    qDebug() << "[MainWindow] Creating folder:" << folderName << "in parent:" << parent_id;
    netManager->createFolder(folderName, parent_id);
}

void MainWindow::handleFolderCreated(bool success, QString message, long long folder_id) {
    qDebug() << "[MainWindow] Folder created:" << success << message << folder_id;
    
    if (success) {
        QMessageBox::information(this, "Create Folder", 
            QString("Folder created successfully!\n\nFolder: %1\nID: %2")
            .arg(message)
            .arg(folder_id));
        
        // File list sẽ tự động refresh từ NetworkManager::createFolder()
    } else {
        QMessageBox::warning(this, "Create Folder", 
            "Failed to create folder:\n" + message);
    }
}

// ===== FOLDER SHARE IMPLEMENTATION =====

void MainWindow::onShareFolderClicked() {
    // Chỉ share được từ "My Files" tab
    if (tabWidget->currentIndex() != 0) {
        QMessageBox::warning(this, "Share Folder", 
            "You can only share folders from 'My Files' tab!");
        return;
    }
    
    // DEMO VERSION: Prompt user để nhập folder_id, folder_name
    // Trong production, bạn sẽ có UI tree view để chọn folder
    
    bool ok;
    
    // Step 1: Nhập folder ID
    QString folderIdStr = QInputDialog::getText(this, "Share Folder",
                                                "Enter Folder ID:",
                                                QLineEdit::Normal, "", &ok);
    if (!ok || folderIdStr.isEmpty()) {
        return;
    }
    
    long long folderId = folderIdStr.toLongLong(&ok);
    if (!ok) {
        QMessageBox::warning(this, "Share Folder", "Invalid folder ID!");
        return;
    }
    
    // Step 2: Nhập folder name
    QString folderName = QInputDialog::getText(this, "Share Folder",
                                               "Enter Folder Name:",
                                               QLineEdit::Normal, "", &ok);
    if (!ok || folderName.isEmpty()) {
        return;
    }
    
    // Step 3: Nhập target username
    QString targetUser = QInputDialog::getText(this, "Share Folder",
                                              QString("Share folder '%1' to username:")
                                              .arg(folderName),
                                              QLineEdit::Normal, "", &ok);
    if (!ok || targetUser.isEmpty()) {
        return;
    }
    
    // Step 4: Chọn local folder path (nơi chứa folder cần upload)
    QString localPath = QFileDialog::getExistingDirectory(this,
                                                          "Select Folder Location",
                                                          QDir::homePath(),
                                                          QFileDialog::ShowDirsOnly);
    if (localPath.isEmpty()) {
        QMessageBox::information(this, "Share Folder", "Folder selection cancelled.");
        return;
    }
    
    // Verify folder exists
    QString fullFolderPath = localPath + "/" + folderName;
    QDir dir(fullFolderPath);
    if (!dir.exists()) {
        QMessageBox::critical(this, "Share Folder", 
            QString("Folder not found at: %1\n\nPlease make sure the folder exists locally.")
            .arg(fullFolderPath));
        return;
    }
    
    // Show folder share dialog
    FolderShareDialog *dialog = new FolderShareDialog(
        folderId, 
        folderName, 
        targetUser, 
        netManager, 
        this
    );
    
    // Set local folder path
    dialog->setLocalFolderPath(localPath);
    
    dialog->exec();
    delete dialog;
}

void MainWindow::showContextMenu(const QPoint &pos) {
    int row = fileTable->rowAt(pos.y());
    
    QMenu menu(this);
    
    // Always show "Create Folder" option
    QAction *createFolderAction = menu.addAction("Create Folder Here");
    connect(createFolderAction, &QAction::triggered, this, &MainWindow::onCreateFolderClicked);
    
    menu.addSeparator();
    
    // If a row is selected, show file/folder specific actions
    if (row >= 0) {
        // NOTE: Trong production, bạn cần detect xem là folder hay file
        // Giả sử thêm data vào QTableWidgetItem
        
        QAction *shareFileAction = menu.addAction("Share File");
        QAction *shareFolderAction = menu.addAction("Share Folder");
        menu.addSeparator();
        QAction *downloadAction = menu.addAction("Download");
        QAction *deleteAction = menu.addAction("Delete");
        
        connect(shareFileAction, &QAction::triggered, this, &MainWindow::onShareClicked);
        connect(shareFolderAction, &QAction::triggered, this, &MainWindow::onShareFolderClicked);
        connect(downloadAction, &QAction::triggered, this, &MainWindow::onDownloadClicked);
        connect(deleteAction, &QAction::triggered, this, &MainWindow::onDeleteClicked);
    }
    
    menu.exec(fileTable->mapToGlobal(pos));
}