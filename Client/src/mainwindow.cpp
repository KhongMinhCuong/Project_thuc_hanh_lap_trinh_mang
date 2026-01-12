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
#include <QCoreApplication>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    netManager = new NetworkManager(this);
    currentFolderId = 0;
    currentFolderPath = "/";
    uploadProgressDialog = nullptr;
    downloadProgressDialog = nullptr;
    setupUI();
    connect(netManager, &NetworkManager::loginSuccess, this, &MainWindow::handleLoginSuccess);
    connect(netManager, &NetworkManager::loginFailed, this, [this](QString msg){
        QMessageBox::critical(this, "Login Error", msg);
    });
    connect(netManager, &NetworkManager::registerSuccess, this, &MainWindow::handleRegisterSuccess);
    connect(netManager, &NetworkManager::registerFailed, this, &MainWindow::handleRegisterFailed);
    connect(netManager, &NetworkManager::fileListReceived, this, &MainWindow::handleFileList);
    connect(netManager, &NetworkManager::connectionStatus, this, [this](bool success, QString msg){
        if(success) QMessageBox::information(this, "Network", msg);
        else QMessageBox::warning(this, "Network", msg);
    });
    connect(netManager, &NetworkManager::uploadStarted, this, &MainWindow::handleUploadStarted);
    connect(netManager, &NetworkManager::uploadProgress, this, &MainWindow::handleUploadProgress);
    connect(netManager, &NetworkManager::folderUploadStarted, this, [this](QString folderName, int totalFiles) {
        if (uploadProgressDialog) {
            delete uploadProgressDialog;
        }
        uploadProgressDialog = new QProgressDialog(QString("Uploading folder: %1 (%2 files)").arg(folderName).arg(totalFiles), QString(), 0, totalFiles, this);
        uploadProgressDialog->setWindowModality(Qt::WindowModal);
        uploadProgressDialog->setMinimumDuration(0);
        uploadProgressDialog->setCancelButton(nullptr);
        uploadProgressDialog->show();
    });
    connect(netManager, &NetworkManager::folderUploadProgress, this, [this](int current, int total, QString fileName) {
        if (uploadProgressDialog) {
            uploadProgressDialog->setValue(current);
            uploadProgressDialog->setLabelText(QString("Uploading: %1 (%2/%3)").arg(fileName).arg(current).arg(total));
        }
    });
    connect(netManager, &NetworkManager::folderUploadCompleted, this, [this](QString folderName) {
        if (uploadProgressDialog) {
            uploadProgressDialog->close();
            delete uploadProgressDialog;
            uploadProgressDialog = nullptr;
        }
        QMessageBox::information(this, "Upload Complete", QString("Folder '%1' uploaded successfully!").arg(folderName));
    });
    connect(netManager, &NetworkManager::downloadStarted, this, &MainWindow::handleDownloadStarted);
    connect(netManager, &NetworkManager::downloadComplete, this, &MainWindow::handleDownloadComplete);
    connect(netManager, &NetworkManager::shareResult, this, &MainWindow::handleShareResult);
    connect(netManager, &NetworkManager::deleteResult, this, &MainWindow::handleDeleteResult);
    connect(netManager, &NetworkManager::renameResult, this, &MainWindow::handleRenameResult);
    connect(netManager, &NetworkManager::logoutSuccess, this, &MainWindow::handleLogout);
}

MainWindow::~MainWindow() {}

void MainWindow::setupUI() {
    stackedWidget = new QStackedWidget(this);
    stackedWidget->addWidget(createLoginPage());
    stackedWidget->addWidget(createDashboardPage());
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
    
    QPushButton *btnConnect = new QPushButton("Connect Server");
    QPushButton *btnLogin = new QPushButton("Login");
    QPushButton *btnRegister = new QPushButton("Register New Account");

    l->addWidget(new QLabel("Server IP:"));
    l->addWidget(hostInput);
    l->addWidget(btnConnect);
    l->addSpacing(20);
    l->addWidget(new QLabel("<b>Login or Register:</b>"));
    l->addWidget(userInput);
    l->addWidget(passInput);
    l->addWidget(btnLogin);
    l->addWidget(btnRegister);
    l->addStretch();

    connect(btnConnect, &QPushButton::clicked, this, &MainWindow::onConnectBtnClicked);
    connect(btnLogin, &QPushButton::clicked, this, &MainWindow::onLoginBtnClicked);
    connect(btnRegister, &QPushButton::clicked, this, &MainWindow::onRegisterBtnClicked);
    return p;
}

QWidget* MainWindow::createDashboardPage() {
    QWidget *p = new QWidget;
    QVBoxLayout *l = new QVBoxLayout(p);
    QHBoxLayout *topBar = new QHBoxLayout;
    QLabel *welcomeLabel = new QLabel("File Management Dashboard");
    QPushButton *btnLogout = new QPushButton("Logout");
    btnLogout->setMaximumWidth(100);
    topBar->addWidget(welcomeLabel);
    topBar->addStretch();
    topBar->addWidget(btnLogout);
    QHBoxLayout *btnLayout = new QHBoxLayout;
    QPushButton *btnRefresh = new QPushButton("Refresh");
    QPushButton *btnUpload = new QPushButton("Upload");
    
    // Create upload menu
    QMenu *uploadMenu = new QMenu(this);
    QAction *uploadFileAction = uploadMenu->addAction("Upload File");
    QAction *uploadFolderAction = uploadMenu->addAction("Upload Folder");
    btnUpload->setMenu(uploadMenu);
    
    btnLayout->addWidget(btnRefresh);
    btnLayout->addWidget(btnUpload);
    btnLayout->addStretch();
    QHBoxLayout *navLayout = new QHBoxLayout;
    backButton = new QPushButton("← Back");
    backButton->setMaximumWidth(80);
    backButton->setEnabled(false);
    
    pathLabel = new QLabel("Home/");
    pathLabel->setStyleSheet(
        "QLabel { font-weight: bold; color: #2c3e50; padding: 5px 10px; background-color: #f5f5f5; border: 1px solid #ddd; border-radius: 4px; }"
    );
    
    breadcrumbLayout = new QHBoxLayout;
    breadcrumbLayout->addWidget(pathLabel);
    breadcrumbLayout->setSpacing(0);
    breadcrumbLayout->setContentsMargins(0, 0, 0, 0);
    
    navLayout->addWidget(backButton);
    navLayout->addLayout(breadcrumbLayout, 1);
    navLayout->addStretch();
    tabWidget = new QTabWidget;
    fileTable = new QTableWidget;
    fileTable->setColumnCount(6);
    fileTable->setHorizontalHeaderLabels({"Filename", "Type", "Size", "Owner", "ID", "Hidden_Type"});
    fileTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    fileTable->setSelectionBehavior(QTableWidget::SelectRows);
    fileTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    fileTable->setColumnHidden(4, true);
    fileTable->setColumnHidden(5, true);
    connect(fileTable, &QTableWidget::cellDoubleClicked, this, &MainWindow::onFolderDoubleClicked);
    fileTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(fileTable, &QTableWidget::customContextMenuRequested, 
            this, &MainWindow::showContextMenu);
    
    sharedFileTable = new QTableWidget;
    sharedFileTable->setColumnCount(6);
    sharedFileTable->setHorizontalHeaderLabels({"Filename", "Type", "Size", "Shared By", "ID", "Hidden_Type"});
    sharedFileTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    sharedFileTable->setSelectionBehavior(QTableWidget::SelectRows);
    sharedFileTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    sharedFileTable->setColumnHidden(4, true);
    sharedFileTable->setColumnHidden(5, true);
    connect(sharedFileTable, &QTableWidget::cellDoubleClicked, this, &MainWindow::onFolderDoubleClicked);
    sharedFileTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(sharedFileTable, &QTableWidget::customContextMenuRequested, 
            this, &MainWindow::showContextMenu);
    
    tabWidget->addTab(fileTable, "My Files");
    tabWidget->addTab(sharedFileTable, "Shared with Me");

    l->addLayout(topBar);
    l->addLayout(btnLayout);
    l->addLayout(navLayout);
    l->addWidget(tabWidget);

    connect(btnRefresh, &QPushButton::clicked, this, &MainWindow::onRefreshClicked);
    connect(backButton, &QPushButton::clicked, this, &MainWindow::onBackButtonClicked);
    connect(uploadFileAction, &QAction::triggered, this, &MainWindow::onUploadFileClicked);
    connect(uploadFolderAction, &QAction::triggered, this, &MainWindow::onUploadFolderClicked);
    connect(btnLogout, &QPushButton::clicked, this, &MainWindow::onLogoutClicked);
    connect(tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);
    
    return p;
}

void MainWindow::onConnectBtnClicked() {
    netManager->connectToServer(hostInput->text(), 8080);
}

void MainWindow::onLoginBtnClicked() {
    netManager->login(userInput->text(), passInput->text());
}

void MainWindow::onRegisterBtnClicked() {
    QString username = userInput->text().trimmed();
    QString password = passInput->text().trimmed();
    
    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "Register", "Please enter both username and password!");
        return;
    }
    
    if (username.length() < 3) {
        QMessageBox::warning(this, "Register", "Username must be at least 3 characters!");
        return;
    }
    
    if (password.length() < 4) {
        QMessageBox::warning(this, "Register", "Password must be at least 4 characters!");
        return;
    }
    
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Confirm Registration", 
                                  QString("Register new account with username: %1?").arg(username),
                                  QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        netManager->registerAccount(username, password);
    }
}

void MainWindow::handleRegisterSuccess(QString msg) {
    QMessageBox::information(this, "Registration Success", 
                            msg + "\n\nYou can now login with your credentials.");
    passInput->clear();
}

void MainWindow::handleRegisterFailed(QString msg) {
    QMessageBox::critical(this, "Registration Failed", msg);
}

void MainWindow::handleLoginSuccess() {
    currentUsername = userInput->text();
    stackedWidget->setCurrentIndex(1);
    netManager->requestFileList();
}

void MainWindow::onRefreshClicked() {
    onTabChanged(tabWidget->currentIndex());
}

void MainWindow::onTabChanged(int index) {
    currentFolderId = 0;
    currentFolderPath = "/";
    folderHistory.clear();
    
    if (index == 0) {
        pathLabel->setText("Home/");
    } else {
        pathLabel->setText("Shared/");
    }
    
    backButton->setEnabled(false);
    
    if (index == 0) {
        netManager->requestFileList(0);
    } else if (index == 1) {
        netManager->requestSharedFileList(-1);
    }
}

void MainWindow::handleFileList(QString data) {
    QTableWidget* targetTable = (tabWidget->currentIndex() == 0) ? fileTable : sharedFileTable;
    targetTable->setRowCount(0);
    
    if (data.isEmpty()) {
        return;
    }
    
    QStringList rows = data.split('\n', Qt::SkipEmptyParts);
    
    for (int i = 0; i < rows.size(); i++) {
        const QString &line = rows[i].trimmed();
        
        if (line.isEmpty()) {
            continue;
        }
        
        QStringList cols = line.split('|');
        
        if (cols.size() >= 5) {
            int row = targetTable->rowCount();
            targetTable->insertRow(row);
            
            QString type = cols[1].trimmed();
            QString sizeStr = cols[2].trimmed();
            
            QString displaySize;
            long long bytes = sizeStr.toLongLong();
            
            if (type == "Folder" && bytes == 0) {
                displaySize = "Empty";
            } else if (bytes < 1024) {
                displaySize = QString::number(bytes) + " B";
            } else if (bytes < 1024 * 1024) {
                displaySize = QString::number(bytes / 1024.0, 'f', 1) + " KB";
            } else if (bytes < 1024 * 1024 * 1024) {
                displaySize = QString::number(bytes / (1024.0 * 1024.0), 'f', 1) + " MB";
            } else {
                displaySize = QString::number(bytes / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " GB";
            }
            
            if (targetTable == fileTable) {
                targetTable->setItem(row, 0, new QTableWidgetItem(cols[0].trimmed()));
                targetTable->setItem(row, 1, new QTableWidgetItem(type));
                targetTable->setItem(row, 2, new QTableWidgetItem(displaySize));
                targetTable->setItem(row, 3, new QTableWidgetItem(cols[3].trimmed()));
                targetTable->setItem(row, 4, new QTableWidgetItem(cols[4].trimmed()));
                targetTable->setItem(row, 5, new QTableWidgetItem(type));
                // Store file_id in hidden column 6
                QTableWidgetItem* fileIdItem = new QTableWidgetItem(cols[4].trimmed());
                targetTable->setItem(row, 6, fileIdItem);
            } else {
                targetTable->setItem(row, 0, new QTableWidgetItem(cols[0].trimmed()));
                targetTable->setItem(row, 1, new QTableWidgetItem(type));
                targetTable->setItem(row, 2, new QTableWidgetItem(displaySize));
                targetTable->setItem(row, 3, new QTableWidgetItem(cols[3].trimmed()));
                targetTable->setItem(row, 4, new QTableWidgetItem(cols[4].trimmed()));
                targetTable->setItem(row, 5, new QTableWidgetItem(type));
                // Store file_id in hidden column 6
                QTableWidgetItem* fileIdItem = new QTableWidgetItem(cols[4].trimmed());
                targetTable->setItem(row, 6, fileIdItem);
            }
        }
    }
}

void MainWindow::onUploadClicked() {
    // Backward compatibility - default to file upload
    onUploadFileClicked();
}

void MainWindow::onUploadFileClicked() {
    QString filePath = QFileDialog::getOpenFileName(this, "Select File to Upload");
    if (!filePath.isEmpty()) {
        netManager->uploadFile(filePath, currentFolderId);
    }
}

void MainWindow::onUploadFolderClicked() {
    QString folderPath = QFileDialog::getExistingDirectory(this, "Select Folder to Upload");
    if (!folderPath.isEmpty()) {
        netManager->uploadFolder(folderPath, currentFolderId);
    }
}

void MainWindow::onDownloadClicked() {
    QTableWidget* currentTable = (tabWidget->currentIndex() == 0) ? fileTable : sharedFileTable;
    
    int row = currentTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Download", "Please select a file first!");
        return;
    }
    
    QString filename = currentTable->item(row, 0)->text();
    
    QString itemType = currentTable->item(row, 5)->text();
    if (itemType == "Folder") {
        QMessageBox::warning(this, "Download", "Cannot download folders!\nPlease select a file.");
        return;
    }
    
    QString savePath = QFileDialog::getSaveFileName(this, 
                                                     "Save File As",
                                                     QDir::homePath() + "/Downloads/" + filename,
                                                     "All Files (*)");
    
    if (!savePath.isEmpty()) {
        netManager->downloadFile(filename, savePath);
    }
}

void MainWindow::onDownloadFolderClicked() {
    QTableWidget* currentTable = (tabWidget->currentIndex() == 0) ? fileTable : sharedFileTable;
    
    int row = currentTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Download Folder", "Please select a folder first!");
        return;
    }
    
    QString folderName = currentTable->item(row, 0)->text();
    
    QString itemType = currentTable->item(row, 5)->text();
    if (itemType != "Folder") {
        QMessageBox::warning(this, "Download Folder", "Please select a folder!");
        return;
    }
    
    QString savePath = QFileDialog::getExistingDirectory(this, 
                                                          "Select Directory to Save Folder",
                                                          QDir::homePath() + "/Downloads");
    
    if (!savePath.isEmpty()) {
        QString fullPath = savePath + "/" + folderName;
        netManager->downloadFolder(folderName, fullPath);
    }
}

void MainWindow::handleUploadStarted(QString filename) {
    if (uploadProgressDialog) {
        delete uploadProgressDialog;
    }
    
    uploadProgressDialog = new QProgressDialog("Uploading: " + filename, QString(), 0, 100, this);
    uploadProgressDialog->setWindowModality(Qt::WindowModal);
    uploadProgressDialog->setMinimumDuration(0);
    uploadProgressDialog->setCancelButton(nullptr);
    uploadProgressDialog->setValue(0);
    uploadProgressDialog->show();
    uploadProgressDialog->raise();
    uploadProgressDialog->activateWindow();
    QCoreApplication::processEvents();
}

void MainWindow::handleUploadProgress(QString msg) {
    if (uploadProgressDialog) {
        uploadProgressDialog->close();
        delete uploadProgressDialog;
        uploadProgressDialog = nullptr;
    }
    
    QMessageBox::information(this, "Upload", msg);
}

void MainWindow::handleDownloadStarted(QString filename) {
    if (downloadProgressDialog) {
        delete downloadProgressDialog;
    }
    
    downloadProgressDialog = new QProgressDialog("Downloading: " + filename, QString(), 0, 100, this);
    downloadProgressDialog->setWindowModality(Qt::WindowModal);
    downloadProgressDialog->setMinimumDuration(0);
    downloadProgressDialog->setCancelButton(nullptr);
    downloadProgressDialog->setValue(0);
    downloadProgressDialog->show();
    downloadProgressDialog->raise();
    downloadProgressDialog->activateWindow();
    QCoreApplication::processEvents();
}

void MainWindow::handleDownloadComplete(QString filename) {
    if (downloadProgressDialog) {
        downloadProgressDialog->close();
        delete downloadProgressDialog;
        downloadProgressDialog = nullptr;
    }
    
    QMessageBox::information(this, "Download", filename);
}

void MainWindow::onShareClicked() {
    if (tabWidget->currentIndex() != 0) {
        QMessageBox::warning(this, "Share", "You can only share from 'My Files' tab!");
        return;
    }
    
    int row = fileTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Share", "Please select a file or folder first!");
        return;
    }
    
    QTableWidgetItem *nameItem = fileTable->item(row, 0);
    QTableWidgetItem *typeItem = fileTable->item(row, 5);
    QTableWidgetItem *idItem = fileTable->item(row, 4);
    
    if (!nameItem || !typeItem) {
        QMessageBox::warning(this, "Share", "Invalid selection!");
        return;
    }
    
    QString itemName = nameItem->text();
    QString itemType = typeItem->text();
    bool isFolder = (itemType == "Folder");
    
    bool ok;
    QString targetUser = QInputDialog::getText(this, "Share " + itemType,
                                              QString("Share %1 '%2' with username:")
                                              .arg(itemType.toLower(), itemName),
                                              QLineEdit::Normal, "", &ok);
    if (!ok || targetUser.isEmpty()) {
        return;
    }
    
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Confirm Share",
                                  QString("Share %1 '%2' with user '%3'?")
                                  .arg(itemType.toLower(), itemName, targetUser),
                                  QMessageBox::Yes | QMessageBox::No);
    
    if (reply != QMessageBox::Yes) {
        return;
    }
    
    qDebug() << "[MainWindow] Starting share process for:" << itemName << "type:" << itemType << "to user:" << targetUser;
    
    if (isFolder) {
        if (!idItem) {
            QMessageBox::warning(this, "Share Folder", "Invalid folder data!");
            return;
        }
        long long folderId = idItem->text().toLongLong();
        qDebug() << "[MainWindow] Sharing folder ID:" << folderId;
        netManager->shareFolderRequest(folderId, targetUser);
    } else {
        qDebug() << "[MainWindow] Sharing file:" << itemName;
        netManager->shareFile(itemName, targetUser);
    }
    // Đã xóa dialog ở đây - sẽ chỉ hiện dialog trong handleShareResult() khi có kết quả từ server
}

void MainWindow::handleShareResult(bool success, QString msg) {
    if (success) {
        QMessageBox::information(this, "Share", msg);
    } else {
        QMessageBox::warning(this, "Share", msg);
    }
}

void MainWindow::onDeleteClicked() {
    if (tabWidget->currentIndex() != 0) {
        QMessageBox::warning(this, "Delete", "You can only delete items from 'My Files' tab!");
        return;
    }
    
    int row = fileTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Delete", "Please select an item first!");
        return;
    }
    
    QTableWidgetItem *nameItem = fileTable->item(row, 0);
    QTableWidgetItem *typeItem = fileTable->item(row, 1);
    
    if (!nameItem || !typeItem) {
        QMessageBox::warning(this, "Delete", "Invalid selection!");
        return;
    }
    
    QString itemName = nameItem->text();
    QString itemType = typeItem->text();
    bool isFolder = (itemType.toLower() == "folder");
    
    // Different dialog messages for file vs folder
    QString title = isFolder ? "Delete Folder" : "Delete File";
    QString message;
    QMessageBox::Icon icon;
    
    if (isFolder) {
        message = QString("Are you sure you want to delete the folder \"%1\"?\n\n"
                         "⚠️ Warning: This will permanently delete the folder and all its contents!")
                         .arg(itemName);
        icon = QMessageBox::Warning;
    } else {
        message = QString("Are you sure you want to delete \"%1\"?").arg(itemName);
        icon = QMessageBox::Question;
    }
    
    QMessageBox msgBox;
    msgBox.setWindowTitle(title);
    msgBox.setText(message);
    msgBox.setIcon(icon);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    
    if (msgBox.exec() == QMessageBox::Yes) {
        netManager->deleteFile(itemName);
    }
}

void MainWindow::handleDeleteResult(bool success, QString msg) {
    if (success) {
        QMessageBox::information(this, "Delete", msg);
        netManager->requestFileList();
    } else {
        QMessageBox::warning(this, "Delete", msg);
    }
}

void MainWindow::onRenameClicked() {
    if (tabWidget->currentIndex() != 0) {
        QMessageBox::warning(this, "Rename", "You can only rename items from 'My Files' tab!");
        return;
    }
    
    int row = fileTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Rename", "Please select a file or folder first!");
        return;
    }
    
    QTableWidgetItem *nameItem = fileTable->item(row, 0);
    QTableWidgetItem *typeItem = fileTable->item(row, 1);
    QTableWidgetItem *fileIdItem = fileTable->item(row, 4);
    
    if (!nameItem || !typeItem || !fileIdItem) {
        QMessageBox::warning(this, "Rename", "Invalid selection!");
        return;
    }
    
    QString oldName = nameItem->text();
    QString itemType = typeItem->text();
    QString fileId = fileIdItem->text();
    
    bool ok;
    QString newName = QInputDialog::getText(this, "Rename " + itemType,
                                           QString("Rename '%1' to:").arg(oldName),
                                           QLineEdit::Normal, oldName, &ok);
    
    if (!ok || newName.isEmpty()) {
        return;
    }
    
    if (newName == oldName) {
        QMessageBox::information(this, "Rename", "Name unchanged.");
        return;
    }
    
    netManager->renameItem(fileId, newName, itemType);
}

void MainWindow::handleRenameResult(bool success, QString msg) {
    if (success) {
        QMessageBox::information(this, "Rename", msg);
        // File list will be refreshed automatically by NetworkManager
    } else {
        QMessageBox::warning(this, "Rename", msg);
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
    fileTable->setRowCount(0);
    sharedFileTable->setRowCount(0);
    
    tabWidget->setCurrentIndex(0);
    
    stackedWidget->setCurrentIndex(0);
    
    passInput->clear();
    
    QMessageBox::information(this, "Logout", "You have been logged out successfully.");
}

void MainWindow::showContextMenu(const QPoint &pos) {
    QTableWidget* sourceTable = qobject_cast<QTableWidget*>(sender());
    if (!sourceTable) return;
    
    int row = sourceTable->rowAt(pos.y());
    
    QMenu menu(this);
    
    menu.addSeparator();
    
    if (row >= 0) {
        QString itemType = sourceTable->item(row, 5)->text();
        
        if (itemType == "Folder") {
            QAction *downloadFolderAction = menu.addAction("Download Folder");
            connect(downloadFolderAction, &QAction::triggered, this, &MainWindow::onDownloadFolderClicked);
        } else {
            QAction *downloadAction = menu.addAction("Download File");
            connect(downloadAction, &QAction::triggered, this, &MainWindow::onDownloadClicked);
        }
        
        if (sourceTable == fileTable) {
            menu.addSeparator();
            QAction *shareAction = menu.addAction("Share");
            QAction *renameAction = menu.addAction("Rename");
            QAction *deleteAction = menu.addAction("Delete");
            
            connect(shareAction, &QAction::triggered, this, &MainWindow::onShareClicked);
            connect(renameAction, &QAction::triggered, this, &MainWindow::onRenameClicked);
            connect(deleteAction, &QAction::triggered, this, &MainWindow::onDeleteClicked);
        }
    }
    
    menu.exec(sourceTable->mapToGlobal(pos));
}

void MainWindow::onFolderDoubleClicked(int row, int column) {
    Q_UNUSED(column);
    
    QTableWidget* sourceTable = qobject_cast<QTableWidget*>(sender());
    if (!sourceTable) return;
    
    QTableWidgetItem *typeItem = sourceTable->item(row, 5);
    QTableWidgetItem *idItem = sourceTable->item(row, 4);
    QTableWidgetItem *nameItem = sourceTable->item(row, 0);
    
    if (!typeItem || !idItem || !nameItem) return;
    
    QString itemType = typeItem->text();
    if (itemType != "Folder") {
        return;
    }
    
    long long folderId = idItem->text().toLongLong();
    QString folderName = nameItem->text();
    
    // Push current folder to history before navigating
    folderHistory.push(currentFolderId);
    
    currentFolderId = folderId;
    
    if (currentFolderPath == "/") {
        currentFolderPath = "/" + folderName;
    } else {
        currentFolderPath += "/" + folderName;
    }
    
    // Update path display based on current tab
    QString prefix = (sourceTable == fileTable) ? "Home/" : "Shared/";
    QString displayPath = prefix + currentFolderPath.mid(1);
    pathLabel->setText(displayPath);
    pathLabel->show();
    
    backButton->setEnabled(true);
    
    if (sourceTable == fileTable) {
        qDebug() << "[MainWindow] Navigating into My Files folder:" << folderId;
        netManager->requestFileList(folderId);
    } else if (sourceTable == sharedFileTable) {
        qDebug() << "[MainWindow] Navigating into Shared folder:" << folderId;
        netManager->requestSharedFileList(folderId);
    }
}

void MainWindow::onBackButtonClicked() {
    QStringList pathParts = currentFolderPath.split('/', Qt::SkipEmptyParts);
    
    if (pathParts.isEmpty()) {
        return;
    }
    
    pathParts.removeLast();
    
    // Determine prefix based on current tab
    QString prefix = (tabWidget->currentIndex() == 0) ? "Home/" : "Shared/";
    
    if (pathParts.isEmpty()) {
        currentFolderPath = "/";
        backButton->setEnabled(false);
        pathLabel->setText(prefix);
        
        // Pop from history to get parent folder ID
        if (!folderHistory.isEmpty()) {
            currentFolderId = folderHistory.pop();
        } else {
            currentFolderId = 0;
        }
    } else {
        currentFolderPath = "/" + pathParts.join("/");
        QString displayPath = prefix + currentFolderPath.mid(1);
        pathLabel->setText(displayPath);
        backButton->setEnabled(true);
        
        // Pop from history to get parent folder ID
        if (!folderHistory.isEmpty()) {
            currentFolderId = folderHistory.pop();
        } else {
            currentFolderId = 0;
        }
    }
    pathLabel->show();
    
    if (tabWidget->currentIndex() == 0) {
        qDebug() << "[MainWindow] Going back in My Files to:" << currentFolderId;
        netManager->requestFileList(currentFolderId);
    } else {
        qDebug() << "[MainWindow] Going back in Shared Files to root";
        currentFolderId = -1;
        netManager->requestSharedFileList(-1);
    }
}

void MainWindow::onBreadcrumbClicked() {
    // Not used anymore
}
