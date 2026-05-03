#!/bin/bash

# DFS Admin CLI Tool
# Provides administrative operations for the distributed file system

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
CONFIG_FILE="$PROJECT_ROOT/config/dfs.conf"
METADATA_FILE="$PROJECT_ROOT/database/metadata.txt"
LASTSEEN_FILE="$PROJECT_ROOT/database/lastseen.csv"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_header() {
    echo -e "${BLUE}================================${NC}"
    echo -e "${BLUE}   DFS Admin CLI Tool${NC}"
    echo -e "${BLUE}================================${NC}"
    echo ""
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

check_status() {
    echo "Checking DFS Status..."
    echo ""
    
    # Check metadata server
    METADATA_PORT=$(grep "^listen_port=" "$CONFIG_FILE" | cut -d'=' -f2)
    if nc -z localhost "$METADATA_PORT" 2>/dev/null; then
        print_success "Metadata server is running on port $METADATA_PORT"
    else
        print_error "Metadata server is not running on port $METADATA_PORT"
    fi
    
    # Check data servers
    if [ -f "$LASTSEEN_FILE" ]; then
        echo ""
        echo "Data Servers:"
        while IFS=',' read -r port timestamp; do
            if [ "$port" != "port" ]; then
                if nc -z localhost "$port" 2>/dev/null; then
                    print_success "Data server on port $port - Last seen: $(date -d @$timestamp 2>/dev/null || echo $timestamp)"
                else
                    print_warning "Data server on port $port - Not responding"
                fi
            fi
        done < "$LASTSEEN_FILE"
    else
        print_warning "No heartbeat data available"
    fi
    
    # Check HTTP server
    if nc -z localhost 8080 2>/dev/null; then
        print_success "HTTP server is running on port 8080"
    else
        print_warning "HTTP server is not running on port 8080"
    fi
}

list_files() {
    echo "Files in DFS:"
    echo ""
    
    if [ ! -f "$METADATA_FILE" ]; then
        print_error "Metadata file not found"
        return
    fi
    
    awk '/^filename/ {
        filename=$2
        getline
        if ($1 == "no_block") blocks=$2
        printf "%-30s %d blocks\n", filename, blocks
    }' "$METADATA_FILE"
}

show_file_info() {
    if [ $# -lt 1 ]; then
        echo "Usage: file_info <filename>"
        return
    fi
    
    local filename="$1"
    echo "File Information: $filename"
    echo ""
    
    awk -v fname="$filename" '
    /^filename/ && $2 == fname {
        found=1
        print "Filename:", $2
    }
    found && /^no_block/ {
        print "Total Blocks:", $2
    }
    found && /^blockid/ {
        block=$2
        print "\nBlock", block":"
    }
    found && /^locations/ {
        print "  Locations:", $2
    }
    found && /^ports/ {
        print "  Ports:", $2
    }
    found && /^filename/ && $2 != fname {
        exit
    }
    ' "$METADATA_FILE"
}

disk_usage() {
    echo "Disk Usage:"
    echo ""
    
    echo "Database Directory:"
    du -h "$PROJECT_ROOT/database" 2>/dev/null | tail -1
    
    echo ""
    echo "Data Blocks:"
    if [ -d "$PROJECT_ROOT/database/log" ]; then
        du -sh "$PROJECT_ROOT/database/log" 2>/dev/null
        echo ""
        echo "Block count: $(find "$PROJECT_ROOT/database/log" -type f 2>/dev/null | wc -l)"
    else
        print_warning "Log directory not found"
    fi
    
    echo ""
    echo "User Directories:"
    if [ -d "$PROJECT_ROOT/database/users" ]; then
        du -sh "$PROJECT_ROOT/database/users"/* 2>/dev/null || echo "No user directories"
    fi
}

list_users() {
    echo "Registered Users:"
    echo ""
    
    if [ ! -f "$PROJECT_ROOT/config/users.csv" ]; then
        print_error "Users file not found"
        return
    fi
    
    printf "%-20s %-40s\n" "Username" "Root Path"
    printf "%-20s %-40s\n" "--------" "---------"
    
    tail -n +2 "$PROJECT_ROOT/config/users.csv" | while IFS=',' read -r username password path; do
        printf "%-20s %-40s\n" "$username" "$path"
    done
}

cleanup_orphans() {
    echo "Cleaning up orphaned blocks..."
    
    # This is a placeholder - actual implementation would need to:
    # 1. Read all block IDs from metadata
    # 2. Check which files exist in log directory
    # 3. Delete files not referenced in metadata
    
    print_warning "Not implemented - manual cleanup required"
    echo ""
    echo "To manually cleanup:"
    echo "1. List all block IDs in metadata.txt"
    echo "2. Compare with files in database/log/"
    echo "3. Remove unreferenced files"
}

verify_integrity() {
    echo "Verifying data integrity..."
    echo ""
    
    # Check if all metadata files exist
    if [ ! -f "$METADATA_FILE" ]; then
        print_error "Metadata file missing: $METADATA_FILE"
    else
        print_success "Metadata file exists"
    fi
    
    if [ ! -f "$LASTSEEN_FILE" ]; then
        print_warning "Last seen file missing: $LASTSEEN_FILE"
    else
        print_success "Last seen file exists"
    fi
    
    # Check database directories
    [ -d "$PROJECT_ROOT/database" ] && print_success "Database directory exists" || print_error "Database directory missing"
    [ -d "$PROJECT_ROOT/database/log" ] && print_success "Log directory exists" || print_error "Log directory missing"
    [ -d "$PROJECT_ROOT/database/users" ] && print_success "Users directory exists" || print_warning "Users directory missing"
    
    echo ""
    echo "Configuration:"
    [ -f "$CONFIG_FILE" ] && print_success "Config file exists" || print_error "Config file missing"
    [ -f "$PROJECT_ROOT/config/users.csv" ] && print_success "Users file exists" || print_warning "Users file missing"
}

show_logs() {
    local service="${1:-all}"
    
    echo "Recent logs (last 20 lines):"
    echo ""
    
    case "$service" in
        metadata)
            print_warning "Log viewing not implemented - check terminal output"
            ;;
        data)
            print_warning "Log viewing not implemented - check terminal output"
            ;;
        *)
            print_warning "Log viewing not implemented"
            echo "Services output to stdout/stderr"
            echo "Redirect output when starting services:"
            echo "  ./build/metaser config/dfs.conf > metadata.log 2>&1"
            ;;
    esac
}

show_menu() {
    print_header
    echo "Available Commands:"
    echo ""
    echo "  status       - Check system status"
    echo "  files        - List all files in DFS"
    echo "  info <file>  - Show detailed file information"
    echo "  users        - List registered users"
    echo "  disk         - Show disk usage statistics"
    echo "  verify       - Verify system integrity"
    echo "  cleanup      - Clean up orphaned blocks"
    echo "  logs [svc]   - Show recent logs"
    echo "  backup       - Create system backup"
    echo "  help         - Show this menu"
    echo "  exit         - Exit admin CLI"
    echo ""
}

# Main execution
if [ $# -eq 0 ]; then
    # Interactive mode
    show_menu
    
    while true; do
        echo -n "dfs-admin> "
        read -r cmd arg1 arg2
        
        case "$cmd" in
            status)
                check_status
                ;;
            files)
                list_files
                ;;
            info)
                show_file_info "$arg1"
                ;;
            users)
                list_users
                ;;
            disk)
                disk_usage
                ;;
            verify)
                verify_integrity
                ;;
            cleanup)
                cleanup_orphans
                ;;
            logs)
                show_logs "$arg1"
                ;;
            backup)
                "$SCRIPT_DIR/backup.sh"
                ;;
            help)
                show_menu
                ;;
            exit|quit)
                echo "Goodbye!"
                exit 0
                ;;
            "")
                ;;
            *)
                print_error "Unknown command: $cmd"
                echo "Type 'help' for available commands"
                ;;
        esac
        echo ""
    done
else
    # Non-interactive mode
    case "$1" in
        status) check_status ;;
        files) list_files ;;
        info) show_file_info "$2" ;;
        users) list_users ;;
        disk) disk_usage ;;
        verify) verify_integrity ;;
        cleanup) cleanup_orphans ;;
        logs) show_logs "$2" ;;
        backup) "$SCRIPT_DIR/backup.sh" ;;
        help) show_menu ;;
        *) 
            print_error "Unknown command: $1"
            show_menu
            exit 1
            ;;
    esac
fi
