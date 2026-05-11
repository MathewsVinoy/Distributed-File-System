# Build stage
FROM gcc:12 AS builder

WORKDIR /build

# Install dependencies
RUN apt-get update && apt-get install -y \
    libssl-dev \
    make \
    && rm -rf /var/lib/apt/lists/*

# Copy source code
COPY . .

# Build the project
RUN make all

# Runtime stage
FROM debian:bookworm-slim

WORKDIR /app

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libssl3 \
    && rm -rf /var/lib/apt/lists/*

# Copy binaries from builder
COPY --from=builder /build/build /app/build
COPY --from=builder /build/config /app/config
COPY --from=builder /build/webclient /app/webclient

# Create necessary directories
RUN mkdir -p /app/database/log /app/database/users /app/out/cli

# Expose ports
# Metadata server: 9000 (client), 9001 (heartbeat)
# Data servers: 8000-8010
# HTTP server: 8080
EXPOSE 9000 9001 8000 8001 8002 8003 8004 8005 8006 8007 8008 8009 8010 8080

# Default command (can be overridden)
CMD ["/bin/bash"]
