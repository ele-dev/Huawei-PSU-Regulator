# Base image for Raspberry Pi 4
FROM dockcross/linux-arm64:latest

# Install additional development packages in the *cross* environment
# IMPORTANT: These packages must exist in Debian aarch64 repos
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        pkg-config \
        libcurl4-openssl-dev \
        nlohmann-json3-dev \
        libmodbus-dev && \
    apt-get clean && rm -rf /var/lib/apt/lists/*
