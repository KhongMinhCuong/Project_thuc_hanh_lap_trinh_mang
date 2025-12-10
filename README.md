# File Management Application

Ứng dụng quản lý file Client-Server đa luồng với giao diện Qt6 và cơ sở dữ liệu MySQL.

## Kiến trúc hệ thống

- **Server:** C++ với MySQL, epoll-based I/O multiplexing
- **Client:** Qt6 GUI application
- **Protocol:** FTP-inspired custom protocol
- **Thread Model:** Fixed Worker Pool (4 threads) + Dedicated I/O Threads

## Tính năng

- ✅ Upload/Download file
- ✅ Chia sẻ file giữa các user
- ✅ Xóa file (soft delete)
- ✅ Quản lý quota người dùng
- ✅ GUI 2 tab: "My Files" và "Shared with Me"
- ✅ Load balancing với least-connections algorithm

## Yêu cầu hệ thống

### Server
- **CMake** >= 3.10
- **GCC** 8+ (hỗ trợ C++17)
- **MySQL** 8.0+
- **OpenSSL** development libraries

```bash
# Ubuntu/Debian
sudo apt install cmake g++ libmysqlclient-dev libssl-dev mysql-server

# Fedora/RHEL
sudo dnf install cmake gcc-c++ mysql-devel openssl-devel
```

### Client
- **Qt6** (Widgets, Network)

```bash
# Ubuntu/Debian
sudo apt install qt6-base-dev

# Fedora/RHEL
sudo dnf install qt6-qtbase-devel
```

## Hướng dẫn cài đặt

### 1. Clone repository

```bash
git clone <your-repo-url>
cd File_Management_App
```

### 2. Thiết lập cơ sở dữ liệu

```bash
cd database
chmod +x setup_database.sh
./setup_database.sh
```

Script này sẽ:
- Tạo database MySQL và user
- Import schema (tables: USERS, FILES, SHAREDFILES, etc.)
- Tạo file `Server/Core/include/db_config.h` với thông tin đăng nhập của bạn

**⚠️ Lưu ý:** File `db_config.h` đã được thêm vào `.gitignore` - không được commit file này!

Nếu cần tùy chỉnh thủ công, xem file template tại `Server/Core/include/db_config.h.template`

### 3. Build và chạy

#### Server

```bash
./run_server.sh
```

#### Client

```bash
./run_client.sh
```

Script tự động build trước khi chạy.

## Sử dụng ứng dụng

1. **Khởi động Server** (cổng 8080)
2. **Mở Client** và kết nối:
   - Host: `localhost` hoặc `127.0.0.1`
   - Port: `8080`
3. **Đăng nhập** với tài khoản mặc định:
   - **admin** / 123456 (quota: 1GB)
   - **guest** / guest (quota: 100MB)

## Cấu hình

### Server

Chỉnh sửa `Server/Core/include/server_config.h`:

```cpp
FIXED_WORKER_THREADS = 4        // Số worker threads (fixed pool)
MAX_DEDICATED_THREADS = 100     // Số thread xử lý file I/O đồng thời
SERVER_PORT = 8080              // Cổng server
STORAGE_PATH = "Server/storage/"// Thư mục lưu file
```

### Database

File cấu hình: `Server/Core/include/db_config.h` (được tạo bởi `setup_database.sh`)

```cpp
#define DB_HOST "localhost"
#define DB_USER "your_username"
#define DB_PASS "your_password"
#define DB_NAME "file_management"
#define DB_PORT 3306
```

## Protocol Commands

| Lệnh | Mô tả |
|------|-------|
| USER \<username\> | Đăng nhập username |
| PASS \<password\> | Xác thực mật khẩu |
| LIST | Liệt kê file của tôi |
| LISTSHARED | Liệt kê file được chia sẻ |
| STOR \<filename\> | Upload file |
| RETR \<filename\> | Download file |
| SHARE \<file\> \<user\> | Chia sẻ file |
| DELETE \<filename\> | Xóa file (soft delete) |
| SITE QUOTA_CHECK \<size\> | Kiểm tra quota |

## Cấu trúc dự án

```
.
├── build/              # Build directory cho Server
├── Client/             # Mã nguồn Client (Qt6)
│   ├── include/        # Header files
│   ├── src/            # Source files và modules
│   ├── UI/             # Qt resource files
│   └── CMakeLists.txt
├── Server/             # Mã nguồn Server
│   ├── Core/
│   │   ├── include/    # Headers (server_config.h, db_config.h, ...)
│   │   └── src/        # Implementation (db/, handler/, thread/)
│   ├── storage/        # Uploaded files (gitignored)
│   └── CMakeLists.txt
├── Common/             # Protocol definitions
│   └── Protocol.h
├── database/           # Database setup
│   ├── schema.sql      # MySQL schema
│   └── setup_database.sh  # Auto-setup script
├── run_server.sh       # Build & run server
└── run_client.sh       # Build & run client
```

## Troubleshooting

### "Cannot connect to database"
- Kiểm tra MySQL đang chạy: `sudo systemctl status mysql`
- Xác minh credentials trong `db_config.h`
- Chạy lại script: `./database/setup_database.sh`

### "Permission denied"
- Cấp quyền thực thi: `chmod +x run_server.sh run_client.sh`
- Kiểm tra quyền thư mục `Server/storage/`

### Build fails
- Kiểm tra CMake version: `cmake --version` (cần >= 3.10)
- Cài đặt đầy đủ dependencies (xem phần Yêu cầu hệ thống)
- Xóa build cache: `rm -rf build Client/build`

### Server không nhận kết nối
- Kiểm tra firewall: `sudo ufw status`
- Kiểm tra port 8080 đã mở: `sudo netstat -tuln | grep 8080`

## Kiến trúc Thread

### AcceptorThread
- Lắng nghe kết nối trên port 8080
- Quản lý fixed pool 4 WorkerThreads
- Load balancing: chọn worker có ít kết nối nhất

### WorkerThread Pool (4 threads)
- Xử lý các kết nối client bằng epoll
- Mỗi worker có thể xử lý nhiều kết nối đồng thời
- Không tự động scale (fixed pool)

### DedicatedThread (on-demand)
- Tạo khi cần upload/download file lớn
- Giới hạn tối đa 100 threads đồng thời
- Tự động cleanup sau khi hoàn thành

### ThreadMonitor
- Giám sát và in thống kê mỗi 30 giây
- Cleanup các dedicated threads đã kết thúc
- Không can thiệp vào worker pool (passive monitoring)
