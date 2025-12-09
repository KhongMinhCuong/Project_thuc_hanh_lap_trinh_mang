#!/bin/bash

# Script to package homework submission
# Usage: ./package.sh [HotenSV] [MSSV]

if [ $# -ne 2 ]; then
    echo "Usage: $0 <HotenSV> <MSSV>"
    echo "Example: $0 TranNguyenNgoc 20161234"
    exit 1
fi

HOTEN=$1
MSSV=$2
PACKAGE_NAME="${HOTEN}_${MSSV}_HW4.zip"

echo "Creating package: $PACKAGE_NAME"
echo "Including files: resolver.c resolver_v2.c Makefile"

# Clean up old executables and logs first
make clean 2>/dev/null || true

# Create the zip package
zip -r "$PACKAGE_NAME" resolver.c resolver_v2.c Makefile

echo "Package created successfully: $PACKAGE_NAME"
echo "File size: $(ls -lh "$PACKAGE_NAME" | awk '{print $5}')"
echo "Contents:"
unzip -l "$PACKAGE_NAME"