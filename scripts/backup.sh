#!/bin/bash

# DFS Backup Script
# Usage: ./backup.sh [backup_dir]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BACKUP_DIR="${1:-$PROJECT_ROOT/backups}"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
BACKUP_NAME="dfs_backup_$TIMESTAMP"
BACKUP_PATH="$BACKUP_DIR/$BACKUP_NAME"

echo "=== DFS Backup Script ==="
echo "Backup destination: $BACKUP_PATH"

# Create backup directory
mkdir -p "$BACKUP_PATH"

# Backup database files
echo "Backing up database..."
if [ -d "$PROJECT_ROOT/database" ]; then
    cp -r "$PROJECT_ROOT/database" "$BACKUP_PATH/"
    echo "✓ Database backed up"
else
    echo "⚠ Database directory not found"
fi

# Backup configuration
echo "Backing up configuration..."
if [ -d "$PROJECT_ROOT/config" ]; then
    cp -r "$PROJECT_ROOT/config" "$BACKUP_PATH/"
    echo "✓ Configuration backed up"
else
    echo "⚠ Configuration directory not found"
fi

# Backup metadata
echo "Backing up metadata..."
if [ -f "$PROJECT_ROOT/database/metadata.txt" ]; then
    cp "$PROJECT_ROOT/database/metadata.txt" "$BACKUP_PATH/"
    echo "✓ Metadata backed up"
fi

# Create manifest
cat > "$BACKUP_PATH/MANIFEST.txt" << EOF
DFS Backup Manifest
==================
Backup Date: $(date)
Backup Version: 1.0
Hostname: $(hostname)
User: $(whoami)

Contents:
- database/: All file data and logs
- config/: Configuration files
- metadata.txt: File metadata

Restore Instructions:
1. Stop all DFS services
2. Run: ./scripts/restore.sh $BACKUP_PATH
3. Restart DFS services
EOF

# Create compressed archive
echo "Creating compressed archive..."
cd "$BACKUP_DIR"
tar -czf "${BACKUP_NAME}.tar.gz" "$BACKUP_NAME"
ARCHIVE_SIZE=$(du -h "${BACKUP_NAME}.tar.gz" | cut -f1)

# Remove uncompressed backup
rm -rf "$BACKUP_NAME"

echo ""
echo "✓ Backup completed successfully!"
echo "Archive: $BACKUP_DIR/${BACKUP_NAME}.tar.gz"
echo "Size: $ARCHIVE_SIZE"
echo ""
echo "To restore this backup, run:"
echo "  ./scripts/restore.sh $BACKUP_DIR/${BACKUP_NAME}.tar.gz"
