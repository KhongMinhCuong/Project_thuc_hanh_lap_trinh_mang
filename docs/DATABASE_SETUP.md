# Database Setup Solution

## Vấn đề

Người dùng khác sau khi pull code từ git về không thể chạy server vì:
- Database credentials bị hardcoded trong `DBManager.cpp`
- Không có database `file_management` trên máy của họ
- Không có user `cuong` với password `040424`

## Giải pháp

### 1. Tách credentials ra file riêng

**File: `Server/Core/include/db_config.h.template`**
```cpp
#ifndef DB_CONFIG_H
#define DB_CONFIG_H

#define DB_HOST "localhost"
#define DB_USER "your_username"
#define DB_PASS "your_password"
#define DB_NAME "file_management"
#define DB_PORT 3306

#endif
```

- User copy file này thành `db_config.h` và điền thông tin của mình
- Hoặc chạy `setup_database.sh` để tự động tạo

### 2. Cập nhật DBManager sử dụng db_config.h

**File: `Server/Core/src/db/DBManager.cpp`**

Thay đổi:
```cpp
// CŨ (hardcoded):
mysql_real_connect(conn, "localhost", "cuong", "040424", 
                  "file_management", 0, nullptr, 0)

// MỚI (sử dụng defines):
#include "../../include/db_config.h"

mysql_real_connect(conn, DB_HOST, DB_USER, DB_PASS, 
                  DB_NAME, DB_PORT, nullptr, 0)
```

### 3. Script tự động setup database

**File: `database/setup_database.sh`**

Script này:
1. Hỏi thông tin MySQL root password
2. Hỏi tên database muốn tạo (default: `file_management`)
3. Hỏi username/password cho database user
4. Tạo database và user
5. Import `schema.sql` (5 tables, 2 users mặc định, ROOT folder)
6. **Tự động tạo file `db_config.h`** với thông tin vừa nhập

Cách dùng:
```bash
cd database
chmod +x setup_database.sh
./setup_database.sh
```

### 4. Bảo vệ credentials khỏi git

**File: `.gitignore`**

```ignore
# Database config (contains sensitive credentials)
Server/Core/include/db_config.h

# Storage (uploaded files)
Server/storage/*
!Server/storage/.gitkeep
```

- `db_config.h` không bao giờ được commit
- Template `db_config.h.template` được commit để làm mẫu

## Kết quả

### Developer mới chỉ cần:

1. Clone repo:
```bash
git clone <repo>
cd File_Management_App
```

2. Chạy setup script:
```bash
cd database
./setup_database.sh
```

3. Nhập thông tin MySQL của họ (không phải của bạn!)

4. Chạy server:
```bash
cd ..
./run_server.sh
```

### Không cần:
- ❌ Sửa code trong `DBManager.cpp`
- ❌ Biết credentials MySQL của người khác
- ❌ Thủ công chạy các lệnh MySQL
- ❌ Thủ công tạo `db_config.h`

## Files thay đổi

### Mới tạo:
1. `Server/Core/include/db_config.h.template` - Template cho config
2. `database/setup_database.sh` - Script tự động setup
3. `.gitignore` - Bảo vệ credentials
4. `Server/storage/.gitkeep` - Giữ thư mục trong git
5. `QUICKSTART.md` - Hướng dẫn nhanh

### Đã sửa:
1. `Server/Core/src/db/DBManager.cpp`
   - Thêm: `#include "../../include/db_config.h"`
   - Thay hardcoded strings thành defines: `DB_HOST`, `DB_USER`, `DB_PASS`, `DB_NAME`, `DB_PORT`

2. `README.md`
   - Thêm phần "Thiết lập cơ sở dữ liệu"
   - Thêm phần "Cấu hình"
   - Thêm phần "Troubleshooting"
   - Cập nhật cấu trúc dự án

## Bảo mật

### Trước:
- ❌ Credentials hardcoded trong source code
- ❌ Password được commit vào git history
- ❌ Mọi người dùng chung 1 database user

### Sau:
- ✅ Credentials tách riêng, không commit
- ✅ Mỗi developer có credentials riêng
- ✅ `.gitignore` bảo vệ `db_config.h`
- ✅ Template file chỉ có placeholders

## Testing

### Kiểm tra setup script hoạt động:

```bash
cd database
./setup_database.sh
```

Nhập:
- MySQL root password: `<your_root_password>`
- Database name: `file_management` (Enter)
- Database user: `fileserver` (Enter)
- Database password: `<choose_a_password>`
- Database host: `localhost` (Enter)

Kết quả mong đợi:
```
✓ Database 'file_management' created successfully
✓ User 'fileserver' created and granted privileges
✓ Schema imported successfully (5 tables created)
✓ Database config written to Server/Core/include/db_config.h

Setup complete! You can now build and run the server.
```

### Kiểm tra server kết nối được:

```bash
./run_server.sh
```

Output mong đợi:
```
[DB] Connected to MySQL database 'file_management'
[Server] Server khởi động trên cổng 8080
```

Nếu thấy lỗi:
```
[DB] Connection failed: Access denied for user...
```

→ Chạy lại `./database/setup_database.sh` với thông tin đúng

## Best Practices

### Cho Developer:
1. **Không bao giờ** commit `db_config.h`
2. **Không bao giờ** hardcode credentials trong code
3. Dùng `setup_database.sh` thay vì tự tạo database
4. Đọc `QUICKSTART.md` trước khi bắt đầu

### Cho Production:
1. Đổi password mặc định của admin/guest
2. Tạo user database riêng (không dùng root)
3. Cấu hình SSL/TLS cho MySQL connection
4. Backup database thường xuyên

## Migration từ old code

Nếu bạn đã có code cũ với hardcoded credentials:

```bash
# 1. Pull code mới
git pull

# 2. Chạy setup script
cd database
./setup_database.sh

# 3. Build lại (db_config.h đã được tạo)
cd ..
./run_server.sh
```

File `db_config.h` cũ (nếu có) sẽ bị ghi đè bởi script.
