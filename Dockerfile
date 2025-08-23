# Multi-stage Dockerfile for Hyperion AI Ultra-Lightweight Framework
# Optimized for minimal image size while maintaining full functionality

# Stage 1: Build environment
FROM ubuntu:22.04 AS builder

# Set build arguments
ARG BUILD_TYPE=Release
ARG ENABLE_SIMD=ON
ARG ENABLE_TESTS=OFF

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    libssl-dev \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /build

# Copy source code
COPY . .

# Create build directory and configure
RUN mkdir -p build && cd build && \
    cmake .. \
    -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    -DBUILD_EXAMPLES=ON \
    -DBUILD_TESTS=${ENABLE_TESTS} \
    -DENABLE_SIMD=${ENABLE_SIMD} \
    -DHYPERION_DOCKER=ON \
    -DCMAKE_INSTALL_PREFIX=/opt/hyperion

# Build Hyperion
RUN cd build && \
    make -j$(nproc) && \
    make install

# Run basic tests if enabled
RUN if [ "${ENABLE_TESTS}" = "ON" ]; then cd build && ctest --output-on-failure; fi

# Stage 2: Runtime environment
FROM ubuntu:22.04 AS runtime

# Install runtime dependencies only
RUN apt-get update && apt-get install -y \
    libssl3 \
    ca-certificates \
    curl \
    && rm -rf /var/lib/apt/lists/* \
    && apt-get clean

# Create non-root user for security
RUN groupadd -r hyperion && useradd -r -g hyperion hyperion

# Set working directory
WORKDIR /opt/hyperion

# Copy built artifacts from builder stage
COPY --from=builder /opt/hyperion/ /opt/hyperion/

# Copy configuration files
COPY docker/hyperion.conf /opt/hyperion/etc/
COPY docker/entrypoint.sh /opt/hyperion/bin/

# Make entrypoint script executable
RUN chmod +x /opt/hyperion/bin/entrypoint.sh

# Create directories for models and data
RUN mkdir -p /opt/hyperion/models /opt/hyperion/data /opt/hyperion/logs && \
    chown -R hyperion:hyperion /opt/hyperion

# Switch to non-root user
USER hyperion

# Expose port for API
EXPOSE 8080

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=60s --retries=3 \
    CMD curl -f http://localhost:8080/health || exit 1

# Set entrypoint
ENTRYPOINT ["/opt/hyperion/bin/entrypoint.sh"]

# Default command
CMD ["server"]

# Stage 3: Development image (optional)
FROM runtime AS development

# Switch back to root for installing dev tools
USER root

# Install development tools
RUN apt-get update && apt-get install -y \
    gdb \
    valgrind \
    strace \
    htop \
    vim \
    && rm -rf /var/lib/apt/lists/*

# Copy source code for debugging
COPY --from=builder /build /opt/hyperion/src

# Switch back to hyperion user
USER hyperion

# Override entrypoint for development
ENTRYPOINT ["/bin/bash"]

# Labels for metadata
LABEL maintainer="Hyperion AI Team"
LABEL version="1.0.0"
LABEL description="Ultra-lightweight AI framework for edge computing"
LABEL org.opencontainers.image.title="Hyperion AI"
LABEL org.opencontainers.image.description="Ultra-lightweight AI inference framework with 4-bit quantization"
LABEL org.opencontainers.image.vendor="Hyperion AI"
LABEL org.opencontainers.image.version="1.0.0"
LABEL org.opencontainers.image.url="https://github.com/TheLakeMan/hyperion"
LABEL org.opencontainers.image.source="https://github.com/TheLakeMan/hyperion"
LABEL org.opencontainers.image.documentation="https://github.com/TheLakeMan/hyperion/blob/main/README.md"
LABEL org.opencontainers.image.licenses="MIT"

# Build-time variables for optimization
ARG BUILDKIT_INLINE_CACHE=1

# Multi-architecture support
# Use --platform flag when building: docker buildx build --platform linux/amd64,linux/arm64

# Size optimization notes:
# - Multi-stage build reduces final image size by ~80%
# - Only runtime dependencies in final image
# - Non-root user for security
# - Health checks for container orchestration
# - Proper label metadata for image management

# Usage examples:
# 
# Build production image:
# docker build -t hyperion-ai:latest .
#
# Build with specific options:
# docker build --build-arg BUILD_TYPE=Release --build-arg ENABLE_SIMD=ON -t hyperion-ai:optimized .
#
# Build development image:
# docker build --target development -t hyperion-ai:dev .
#
# Build multi-architecture:
# docker buildx build --platform linux/amd64,linux/arm64 -t hyperion-ai:multi-arch .
#
# Run container:
# docker run -d -p 8080:8080 --name hyperion hyperion-ai:latest
#
# Run with custom config:
# docker run -d -p 8080:8080 -v /host/config:/opt/hyperion/etc hyperion-ai:latest
#
# Run development container:
# docker run -it --rm -v /host/src:/opt/hyperion/src hyperion-ai:dev