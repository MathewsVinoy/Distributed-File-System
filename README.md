# Distributed File System (DFS)

A high-performance, fault-tolerant distributed file system written in C with support for data replication, TLS encryption, and web-based management.

## Features

- **Data Replication**: Automatic replication across multiple data servers for fault tolerance
- **TLS Encryption**: Secure communication between all components with optional TLS support
- **Web Interface**: Modern web-based client for file management with user authentication
- **Heartbeat Monitoring**: Automatic health checking of data servers
- **Connection Pooling**: Efficient connection management for improved performance
- **Block-based Storage**: Efficient storage using configurable block sizes
- **Thread Pool**: Concurrent request handling for improved throughput

## Architecture

```
┌─────────────────┐
│   Web Client    │
│   (HTTP 8080)   │
└────────┬────────┘
         │
┌────────▼────────┐      ┌──────────────────┐
│  Client/HTTP    │◄────►│ Metadata Server  │
│    Server       │      │   (Port 9000)    │
└────────┬────────┘      └────────┬─────────┘
         │                        │
         │                   Heartbeat
         │                  (Port 9001)
         │                        │
    ┌────▼────────────────────────▼────┐
    │      Data Servers (8000+)        │
    │  ┌──────┐  ┌──────┐  ┌──────┐  │
    │  │ DS 1 │  │ DS 2 │  │ DS 3 │  │
    │  └──────┘  └──────┘  └──────┘  │
    └──────────────────────────────────┘
```

### Components

1. **Metadata Server** (`metaser`): Manages file metadata, block locations, and coordinates writes
2. **Data Servers** (`ser`): Store actual file data blocks with replication
3. **CLI Client** (`cli`): Command-line interface for file operations
4. **HTTP Server** (`client_http`): Web interface with authentication

## Prerequisites

- GCC compiler
- OpenSSL development libraries
- pthreads support
- Make

### Installation

**Ubuntu/Debian:**

```bash
sudo apt-get update
sudo apt-get install build-essential libssl-dev
```

**RHEL/CentOS:**

```bash
sudo yum install gcc openssl-devel
```

**macOS:**

```bash
brew install openssl
```

## Building

```bash
# Build all components
make all

# Build individual components
make cli          # Build CLI client
make metaser      # Build metadata server
make ser          # Build data server
make client_http  # Build HTTP server

# Clean build artifacts
make clean
```

Binaries are created in the `build/` directory.

## Configuration

Edit `config/dfs.conf` to configure the system:

```ini
[global]
block_size=1024                    # Block size in bytes
metadata_file=database/metadata.txt
last_seen_file=database/lastseen.csv

[metadata]
listen_addr=0.0.0.0
listen_port=9000
heartbeat_port=9001
tls_enabled=false

[data]
bind_addr=0.0.0.0
port=8000                          # Base port (increment for multiple servers)
data_file=database/my_file.txt
log_dir=database/log
metadata_host=127.0.0.1
metadata_port=9000

[client]
metadata_host=127.0.0.1
metadata_port=9000
output_file=out/cli/myfile.txt
```

### User Management

Edit `config/users.csv` to add users for the web interface:

```csv
username,password,root_path
alice,password123,database/users/alice
bob,password456,database/users/bob
```

**⚠️ SECURITY WARNING**: Passwords are stored in plain text. Use strong passwords and restrict file permissions:

```bash
chmod 600 config/users.csv
```

## Running the System

### 1. Start Metadata Server

```bash
./build/metaser config/dfs.conf
```

### 2. Start Data Servers

Start multiple data servers on different ports for replication:

```bash
# Terminal 1
./build/ser config/dfs.conf 8000

# Terminal 2
./build/ser config/dfs.conf 8001

# Terminal 3
./build/ser config/dfs.conf 8002
```

### 3. Use the CLI Client

```bash
./build/cli config/dfs.conf
```

Options:

1. **Lookup block**: Query block location
2. **Write block**: Upload data to replicas
3. **Read entire file**: Download complete file

### 4. Start Web Interface

```bash
./build/client_http config/dfs.conf
```

Access at `http://localhost:8080`

## API Reference

### Metadata Server Protocol

**Lookup Block:**

```
LOOKUP <filename> <block_id>
```

**Get File Map:**

```
GET_FILE_MAP <filename>
```

**Write Block:**

```
WRITE_BLOCK <filename> <block_id>
```

### Data Server Protocol

**Store Block:**

```
PUT BLOCK <block_id>
<data>
```

**Retrieve Block:**

```
GET BLOCK <block_id>
```

## File Structure

```
.
├── build/              # Compiled binaries
├── config/
│   ├── dfs.conf       # System configuration
│   └── users.csv      # User credentials
├── database/
│   ├── metadata.txt   # File metadata
│   ├── lastseen.csv   # Server health status
│   ├── log/           # Data server block storage
│   └── users/         # User file storage
├── include/           # Header files
│   ├── common/        # Shared utilities
│   ├── clint/         # Client headers
│   ├── dataserver/    # Data server headers
│   └── metadata/      # Metadata server headers
├── src/               # Source files
│   ├── common/        # Shared code
│   ├── clint_res/     # Client implementation
│   ├── dataserver_res/# Data server implementation
│   └── metadata_res/  # Metadata server implementation
└── webclient/         # Web interface files
```

## TLS Configuration

To enable TLS encryption:

1. Generate certificates:

```bash
# Create CA
openssl genrsa -out ca-key.pem 4096
openssl req -new -x509 -days 365 -key ca-key.pem -out ca.pem

# Create server certificate
openssl genrsa -out server-key.pem 4096
openssl req -new -key server-key.pem -out server.csr
openssl x509 -req -days 365 -in server.csr -CA ca.pem -CAkey ca-key.pem -CAcreateserial -out server-cert.pem
```

2. Update `config/dfs.conf`:

```ini
[metadata]
tls_enabled=true
tls_cert_file=config/certs/metadata_cert.pem
tls_key_file=config/certs/metadata_key.pem
tls_ca_file=config/certs/ca.pem
```

## Troubleshooting

### Connection Refused

- Ensure metadata server is running first
- Check firewall settings
- Verify ports in configuration

### Authentication Failed

- Check `config/users.csv` format
- Ensure username and password match
- Verify file permissions

### Block Not Found

- Verify metadata file exists
- Check data server logs
- Ensure data servers sent heartbeats

### Performance Issues

- Increase thread pool size in code
- Use faster storage for data servers
- Enable connection pooling
- Consider SSD storage for metadata

## Development

### Code Structure

- `common/`: Shared utilities (logging, config, TLS, protocol, thread pool)
- `clint_res/`: Client-side operations (read, write, lookup)
- `metadata_res/`: Metadata management, heartbeat handling
- `dataserver_res/`: Data storage and heartbeat sending

### Adding New Features

1. Update protocol in `include/common/protocol.h`
2. Implement handlers in respective modules
3. Update client code to use new features
4. Test with all components running

## Known Limitations

- No automatic data rebalancing
- No file deletion via CLI (web interface only)
- No file versioning
- Fixed block size per configuration
- No distributed locking for concurrent writes
- Manual failover required

## Performance Tuning

- **Block Size**: Larger blocks reduce metadata overhead but increase minimum transfer size
- **Replication Factor**: More replicas improve availability but increase storage and network overhead
- **Thread Pool**: Adjust in source for higher concurrency
- **Connection Pool**: Reuses connections for better performance

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## License

See [LICENSE.md](LICENSE.md) for details.

## Security Considerations

⚠️ **Important Security Notes:**

1. **Passwords**: Currently stored in plain text - implement hashing before production use
2. **TLS**: Strongly recommended for production deployments
3. **Authentication**: Web interface uses simple token-based auth
4. **File Permissions**: Restrict access to config and database directories
5. **Network**: Use firewall rules to restrict access to trusted hosts

## Future Enhancements

- [ ] Password hashing (bcrypt/argon2)
- [ ] Data integrity checksums
- [ ] Automatic failover and rebalancing
- [ ] File deletion and rename operations
- [ ] Distributed locking mechanism
- [ ] Metrics and monitoring endpoints
- [ ] Docker containerization
- [ ] Backup and restore utilities
- [ ] Admin CLI tools
- [ ] Write-ahead logging for crash recovery

## Support

For issues, questions, or contributions, please open an issue on the repository.
