#!/bin/bash
# Script tự động tạo database và user MySQL

echo "========================================="
echo "FILE MANAGEMENT SERVER - Database Setup"
echo "========================================="
echo ""

# Đọc thông tin từ user
read -p "Enter MySQL root password: " -s MYSQL_ROOT_PASS
echo ""
read -p "Enter new database name [file_management]: " DB_NAME
DB_NAME=${DB_NAME:-file_management}

read -p "Enter new MySQL username [fileserver]: " DB_USER
DB_USER=${DB_USER:-fileserver}

read -p "Enter password for '$DB_USER': " -s DB_PASS
echo ""

read -p "Enter MySQL host [localhost]: " DB_HOST
DB_HOST=${DB_HOST:-localhost}

echo ""
echo "Configuration:"
echo "  Database: $DB_NAME"
echo "  User: $DB_USER"
echo "  Host: $DB_HOST"
echo ""
read -p "Proceed? [y/N]: " confirm

if [[ ! $confirm =~ ^[Yy]$ ]]; then
    echo "Cancelled."
    exit 1
fi

# Tạo database và user
echo ""
echo "[1/3] Creating database and user..."
mysql -h "$DB_HOST" -u root -p"$MYSQL_ROOT_PASS" <<EOF
CREATE DATABASE IF NOT EXISTS $DB_NAME CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
CREATE USER IF NOT EXISTS '$DB_USER'@'localhost' IDENTIFIED BY '$DB_PASS';
GRANT ALL PRIVILEGES ON $DB_NAME.* TO '$DB_USER'@'localhost';
FLUSH PRIVILEGES;
EOF

if [ $? -ne 0 ]; then
    echo "Error: Failed to create database/user"
    exit 1
fi

echo "[2/3] Importing schema..."
mysql -h "$DB_HOST" -u "$DB_USER" -p"$DB_PASS" "$DB_NAME" < "$(dirname "$0")/schema.sql"

if [ $? -ne 0 ]; then
    echo "Error: Failed to import schema"
    exit 1
fi

echo "[3/3] Creating config file..."
cat > "$(dirname "$0")/../Server/Core/include/db_config.h" <<EOF
#ifndef DB_CONFIG_H
#define DB_CONFIG_H

// Auto-generated database configuration
// DO NOT commit this file to git!

#define DB_HOST "$DB_HOST"
#define DB_USER "$DB_USER"
#define DB_PASS "$DB_PASS"
#define DB_NAME "$DB_NAME"
#define DB_PORT 3306

#endif // DB_CONFIG_H
EOF

echo ""
echo "Database setup completed successfully!"
echo ""
echo "Database credentials:"
echo "  Host: $DB_HOST"
echo "  Database: $DB_NAME"
echo "  Username: $DB_USER"
echo "  Password: ********"
echo ""
echo "IMPORTANT: Add 'db_config.h' to .gitignore!"
echo ""
