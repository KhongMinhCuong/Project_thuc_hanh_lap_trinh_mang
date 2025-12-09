# File Management Application

Ứng dụng quản lý file Client-Server sử dụng C++, Qt6 và MySQL.

## Yêu cầu hệ thống

- **CMake** >= 3.16
- **Qt6** (Widgets, Network)
- **MySQL Server** và libmysqlclient-dev
- **G++** với hỗ trợ C++17
- **pkg-config**

## Cài đặt dependencies

```bash
sudo apt update
sudo apt install -y cmake build-essential qt6-base-dev libmysqlclient-dev mysql-server pkg-config
```

## Build dự án

### 1. Build Server

```bash
# Tạo thư mục build
mkdir -p build
cd build

# Chạy CMake
cmake ../Server

# Biên dịch
make

# Quay lại thư mục gốc
cd ..
```

### 2. Build Client

```bash
# Tạo thư mục build cho Client
mkdir -p build_client
cd build_client

# Chạy CMake
cmake ../Client

# Biên dịch
make

# Quay lại thư mục gốc
cd ..
```

## Chạy ứng dụng

### Chạy Server

```bash
# Cách 1: Chạy trực tiếp
cd build
./FileServer

# Cách 2: Sử dụng script
./run_server.sh
```

Server sẽ lắng nghe trên **cổng 8080**.

### Chạy Client

**Lưu ý:** Client yêu cầu môi trường đồ họa (X11/Wayland).

```bash
# Cách 1: Chạy trực tiếp
cd build_client
./FileClient

# Cách 2: Sử dụng script
./run_client.sh
```

## Sử dụng

1. **Khởi động Server** trước
2. **Mở Client** và nhập thông tin:
   - Host: `localhost` hoặc `127.0.0.1`
   - Port: `8080`
3. **Đăng nhập** bằng tài khoản mẫu:
   - Username: `admin` / Password: `123456`
   - Username: `guest` / Password: `guest`

## Cấu trúc dự án

```
.
├── build/              # Build directory cho Server
├── build_client/       # Build directory cho Client
├── Client/             # Mã nguồn Client (Qt6)
│   ├── include/        # Header files
│   ├── src/            # Source files
│   └── CMakeLists.txt
├── Server/             # Mã nguồn Server
│   ├── Core/
│   │   ├── include/    # Header files
│   │   └── src/        # Source files
│   ├── config/         # Configuration files
│   └── CMakeLists.txt
├── Common/             # Shared protocol definitions
│   └── Protocol.h
└── database/           # Database schema
    └── schema.sql
```

## Tính năng hiện tại

- ✅ Kết nối Client-Server qua TCP
- ✅ Xác thực người dùng (mock data)
- ✅ Liệt kê danh sách file
- ✅ Hỗ trợ đa luồng (Worker Thread, Dedicated Thread)
- ✅ I/O Multiplexing (epoll)

## Ghi chú

- Database hiện đang sử dụng **mock data** trong `DBManager.cpp`
- Để kết nối MySQL thực, cần cập nhật code trong `Server/Core/src/db/DBManager.cpp`
- File `schema.sql` hiện đang trống, cần được định nghĩa khi triển khai database thật
