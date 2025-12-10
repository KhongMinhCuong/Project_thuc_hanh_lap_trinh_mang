# Quick Start Guide

Hướng dẫn nhanh để chạy dự án ngay lập tức.

## Bước 1: Cài đặt dependencies

```bash
# Ubuntu/Debian
sudo apt install cmake g++ libmysqlclient-dev libssl-dev mysql-server qt6-base-dev

# Fedora/RHEL  
sudo dnf install cmake gcc-c++ mysql-devel openssl-devel qt6-qtbase-devel
```

## Bước 2: Thiết lập MySQL Database

```bash
cd database
chmod +x setup_database.sh
./setup_database.sh
```

Nhập thông tin khi được hỏi:
- **MySQL root password**: Mật khẩu root của MySQL (để tạo database)
- **Database name**: Nhấn Enter để dùng mặc định `file_management`
- **Database user**: Nhấn Enter để dùng mặc định `fileserver`
- **Database password**: Nhập mật khẩu cho user `fileserver`
- **Database host**: Nhấn Enter để dùng mặc định `localhost`

Script sẽ tự động:
1. Tạo database
2. Tạo user và cấp quyền
3. Import schema (5 tables)
4. Tạo 2 user mặc định (admin, guest)
5. Tạo ROOT folder
6. Tạo file `Server/Core/include/db_config.h`

## Bước 3: Chạy Server

```bash
cd ..
./run_server.sh
```

Output mong đợi:
```
[DB] Connected to MySQL database 'file_management'
[Server] Server khởi động trên cổng 8080
[AcceptorThread] Tạo 4 worker threads...
[ThreadMonitor] Monitor thread started
```

## Bước 4: Chạy Client (terminal mới)

```bash
./run_client.sh
```

## Bước 5: Đăng nhập

Trong GUI client:
1. **Host**: `localhost`
2. **Port**: `8080`
3. **Username**: `admin`
4. **Password**: `123456`

## Tài khoản mặc định

| Username | Password | Quota |
|----------|----------|-------|
| admin    | 123456   | 1GB   |
| guest    | guest    | 100MB |

## Troubleshooting nhanh

### "Cannot connect to database"
```bash
# Kiểm tra MySQL đang chạy
sudo systemctl status mysql

# Nếu không chạy
sudo systemctl start mysql

# Chạy lại setup script
cd database
./setup_database.sh
```

### "Permission denied" khi chạy script
```bash
chmod +x run_server.sh run_client.sh database/setup_database.sh
```

### Build fails
```bash
# Xóa cache và build lại
rm -rf build Client/build
./run_server.sh
```

### Port 8080 đã được sử dụng
```bash
# Tìm process đang dùng port 8080
sudo lsof -i :8080

# Kill process (thay <PID> bằng process ID)
kill <PID>
```

## Các lệnh hữu ích

### Kiểm tra database
```bash
mysql -u fileserver -p
USE file_management;
SHOW TABLES;
SELECT * FROM USERS;
```

### Xem log server (nếu có lỗi)
Server in log ra stdout, chạy với redirect:
```bash
./run_server.sh 2>&1 | tee server.log
```

### Reset database
```bash
cd database
./setup_database.sh
# Chọn Drop existing database khi được hỏi
```

## Kiểm tra kết nối

```bash
# Test kết nối tới server
telnet localhost 8080

# Hoặc dùng netcat
nc localhost 8080
```

Nếu kết nối thành công, bạn có thể test protocol:
```
USER admin
PASS 123456
LIST
```

## Thư mục quan trọng

- `Server/storage/`: Nơi lưu file upload
- `Server/Core/include/db_config.h`: Cấu hình database (không commit!)
- `database/schema.sql`: Database schema
- `build/`: Build files của server
- `Client/build/`: Build files của client

## Xem thêm

Chi tiết đầy đủ: [README.md](README.md)
