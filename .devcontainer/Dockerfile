FROM --platform=linux/amd64 ubuntu:22.04

ARG USERNAME=vscode
ARG USER_UID=1000
ARG USER_GID=${USER_UID}

ENV DEBIAN_FRONTEND=noninteractive \
    TZ=Etc/UTC \
    LANG=C.UTF-8 \
    LC_ALL=C.UTF-8

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        apt-transport-https \
        build-essential \
        ca-certificates \
        clang \
        cmake \
        curl \
        dos2unix \
        file \
        gdb \
        git \
        git-lfs \
        jq \
        less \
        libasound2 \
        libcurl4-openssl-dev \
        libfontconfig1 \
        libfreetype6 \
        libgl1 \
        libglu1-mesa \
        libgtk-3-0 \
        libicu-dev \
        libnss3 \
        libssl-dev \
        libvulkan1 \
        libx11-6 \
        libx11-dev \
        libxcursor1 \
        libxcursor-dev \
        libxext6 \
        libxext-dev \
        libxi6 \
        libxi-dev \
        libxinerama1 \
        libxinerama-dev \
        libxrandr2 \
        libxrandr-dev \
        libxss1 \
        lld \
        locales \
        lsb-release \
        make \
        mesa-vulkan-drivers \
        nano \
        ninja-build \
        patchelf \
        perl \
        pkg-config \
        procps \
        psmisc \
        python3 \
        python3-pip \
        python3-venv \
        rsync \
        sudo \
        tzdata \
        unzip \
        vim-tiny \
        vulkan-tools \
        wget \
        xdg-user-dirs \
        zip \
        zlib1g-dev \
    && git lfs install --system \
    && groupadd --gid ${USER_GID} ${USERNAME} \
    && useradd --uid ${USER_UID} --gid ${USER_GID} -m ${USERNAME} -s /bin/bash \
    && echo "${USERNAME} ALL=(root) NOPASSWD:ALL" > /etc/sudoers.d/${USERNAME} \
    && chmod 0440 /etc/sudoers.d/${USERNAME} \
    && rm -rf /var/lib/apt/lists/*

USER ${USERNAME}
WORKDIR /workspaces
