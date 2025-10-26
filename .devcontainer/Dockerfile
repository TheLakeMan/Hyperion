FROM codercom/code-server:latest

USER root

ARG DEBIAN_FRONTEND=noninteractive

RUN set -eux; \
    apt-get update; \
    apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        ninja-build \
        gcc \
        g++ \
        clang \
        clang-format \
        lld \
        gdb \
        pkg-config \
        python3 \
        python3-venv \
        python3-pip \
        python3-setuptools \
        python3-wheel \
        valgrind \
        lcov \
        gcovr \
        llvm \
        binaryen \
        git \
        curl \
        wget \
        ca-certificates \
        nodejs \
        npm \
        openjdk-17-jre \
        gcc-arm-linux-gnueabihf \
        g++-arm-linux-gnueabihf \
        make \
        rsync \
        zip \
        unzip; \
    rm -rf /var/lib/apt/lists/*

# Install PlatformIO via pip (preferred distribution method)
RUN python3 -m pip install --no-cache-dir --break-system-packages platformio

# Install arduino-cli binary
RUN set -eux; \
    ARDUINO_CLI_VERSION=0.35.3; \
    mkdir -p /tmp/arduino-cli; \
    curl -fsSL "https://downloads.arduino.cc/arduino-cli/arduino-cli_${ARDUINO_CLI_VERSION}_Linux_64bit.tar.gz" -o /tmp/arduino-cli.tar.gz; \
    tar -xzf /tmp/arduino-cli.tar.gz -C /tmp/arduino-cli; \
    mv /tmp/arduino-cli/arduino-cli /usr/local/bin/arduino-cli; \
    chmod +x /usr/local/bin/arduino-cli; \
    rm -rf /tmp/arduino-cli /tmp/arduino-cli.tar.gz

# Install Emscripten SDK
ENV EMSDK_ROOT=/opt/emsdk

RUN set -eux; \
    git clone https://github.com/emscripten-core/emsdk.git "$EMSDK_ROOT"; \
    cd "$EMSDK_ROOT"; \
    ./emsdk install latest; \
    ./emsdk activate latest

# Make Emscripten environment available for all shells
RUN printf '#!/bin/bash\nsource %s/emsdk_env.sh\n' "$EMSDK_ROOT" > /etc/profile.d/emsdk.sh && \
    chmod +x /etc/profile.d/emsdk.sh && \
    printf '\nsource /etc/profile.d/emsdk.sh\n' >> /etc/bash.bashrc

# Pre-create workspace directory for code-server user
RUN mkdir -p /home/coder/project && chown -R coder:coder /home/coder/project

COPY entrypoint.sh /usr/local/bin/entrypoint.sh
COPY code-server-launch.sh /usr/local/bin/code-server-launch.sh
RUN chmod +x /usr/local/bin/entrypoint.sh /usr/local/bin/code-server-launch.sh

USER root

WORKDIR /home/coder/project

ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
