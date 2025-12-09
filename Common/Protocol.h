// File: Common/Protocol.h
#ifndef PROTOCOL_H
#define PROTOCOL_H

// --- Lệnh (Commands) ---
#define CMD_USER "USER"             // Đăng nhập: USER <username>
#define CMD_PASS "PASS"             // Mật khẩu: PASS <password>
#define CMD_REGISTER "REGISTER"     // Đăng ký: REGISTER <username> <password>
#define CMD_LIST "LIST"             // Lấy danh sách file của mình
#define CMD_LISTSHARED "LISTSHARED" // Lấy danh sách file được share
#define CMD_SEARCH "SEARCH"         // Tìm kiếm file: SEARCH <keyword>
#define CMD_SHARE "SHARE"           // Chia sẻ file: SHARE <filename> <target_user>
#define CMD_DELETE "DELETE"         // Xóa file: DELETE <filename>
#define CMD_UPLOAD_CHECK "SITE QUOTA_CHECK" // Kiểm tra dung lượng
#define CMD_UPLOAD "STOR"           // Upload file
#define CMD_DOWNLOAD "RETR"         // Download file

// --- Mã Trạng Thái (Status Codes) ---
#define CODE_OK "200"
#define CODE_LOGIN_SUCCESS "230"    // Đăng nhập thành công [cite: 109]
#define CODE_DATA_OPEN "150"        // Bắt đầu truyền dữ liệu [cite: 59]
#define CODE_TRANSFER_COMPLETE "226"// Truyền xong [cite: 60]
#define CODE_LOGIN_FAIL "530"       // Sai mật khẩu/Chưa login [cite: 109]
#define CODE_FAIL "550"             // Lỗi chung

#endif // PROTOCOL_H