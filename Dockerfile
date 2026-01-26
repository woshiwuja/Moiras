FROM ubuntu:22.04

# Evita prompt interattivi durante l'installazione
ENV DEBIAN_FRONTEND=noninteractive

# Installa le dipendenze di sistema e gli strumenti di build
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    libasound2-dev \
    libx11-dev \
    libxrandr-dev \
    libxi-dev \
    libgl1-mesa-dev \
    libglu1-mesa-dev \
    libxcursor-dev \
    libxinerama-dev \
    libwayland-dev \
    libglfw3-dev \
    libxkbcommon-dev \
    && rm -rf /var/lib/apt/lists/*

# Imposta la directory di lavoro
WORKDIR /app

# Comando di default per aprire una shell
CMD ["/bin/bash"]