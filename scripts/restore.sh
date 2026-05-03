#!/bin/bash

# DFS Restore Script
# Usage: ./restore.sh <backup_archive>

set -e

if [ $# -lt 1 ]; then
    echo "Usage: $0 <backup_archive>"
    echo "Example: $0 backups/dfs_backup_20260306_120000.tar.gz"
    exit 1
fi

BACKUP_ARCHIVE="$1"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
TEMP_DIR=$(mktemp -d)

if [ ! -f "$BACKUP_ARCHIVE" ]; then
    echo "Error: Backup archive not found: $BACKUP_ARCHIVE"
    exit 1
fi

echo "=== DFS Restore Script ==="
echo "Backup archive: $BACKUP_ARCHIVE"
echo "Restore destination: $PROJECT_ROOT"
echo ""

# Confirm restoration
read -p "This will overwrite existing data. Continue? (yes/no): " confirm
if [ "$confirm" != "yes" ]; then
    echo "Restore cancelled"
    exit 0
fi

# Extract archive
echo "Extracting backup archive..."
tar -xzf "$BACKUP_ARCHIVE" -C "$TEMP_DIR"
BACKUP_DIR=$(ls -d "$TEMP_DIR"/dfs_backup_* | head -n 1)

if [ ! -d "$BACKUP_DIR" ]; then
    echo "Error: Invalid backup archive structure"
    rm -rf "$TEMP_DIR"
    exit 1
fi

# Display manifest
if [ -f "$BACKUP_DIR/MANIFEST.txt" ]; then
    echo ""
    echo "=== Backup Manifest ==="
    cat "$BACKUP_DIR/MANIFEST.txt"
    echo ""
fi

# Backup current data (safety backup)
SAFETY_BACKUP="$PROJECT_ROOT/backups/pre_restore_$(date +%Y%m%d_%H%M%S)"
echo "Creating safety backup of current data: $SAFETY_BACKUP"
mkdir -p "$SAFETY_BACKUP"
[ -d "$PROJECT_ROOT/database" ] && cp -r "$PROJECT_ROOT/database" "$SAFETY_BACKUP/" || true
[ -d "$PROJECT_ROOT/config" ] && cp -r "$PROJECT_ROOT/config" "$SAFETY_BACKUP/" || true

# Restore database
if [ -d "$BACKUP_DIR/database" ]; then
    echo "Restoring database..."
    rm -rf "$PROJECT_ROOT/database"
    cp -r "$BACKUP_DIR/database" "$PROJECT_ROOT/"
    echo "✓ Database restored"
fi

# Restore configuration
if [ -d "$BACKUP_DIR/config" ]; then
    echo "Restoring configuration..."
    # Backup current config before overwriting
    if [ -d "$PROJECT_ROOT/config" ]; then
        cp -r "$PROJECT_ROOT/config" "$SAFETY_BACKUP/config.current"
    fi
    rm -rf "$PROJECT_ROOT/config"
    cp -r "$BACKUP_DIR/config" "$PROJECT_ROOT/"
    echo "✓ Configuration restored"
fi

# Set proper permissions
echo "Setting permissions..."
chmod -R u+rw "$PROJECT_ROOT/database" 2>/dev/null || true
chmod -R u+rw "$PROJECT_ROOT/config" 2>/dev/null || true

# Cleanup
rm -rf "$TEMP_DIR"

echo ""
echo "✓ Restore completed successfully!"
echo ""
echo "Safety backup saved to: $SAFETY_BACKUP"
echo ""
echo "Next steps:"
echo "1. Verify configuration in config/dfs.conf"
echo "2. Start metadata server: ./build/metaser config/dfs.conf"
echo "3. Start data servers: ./build/ser config/dfs.conf <port>"
echo "4. Verify system functionality"
