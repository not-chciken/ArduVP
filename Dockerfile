# Development/CI image for ArduVP.
# Published as chciken/arduvp-ci:latest and used as-is by the GitHub CI
# workflow. Rebuild and push after changing this file:
#   docker build -t chciken/arduvp-ci:latest .
#   docker push chciken/arduvp-ci:latest

FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    git \
    ca-certificates \
    python3 \
    python3-pexpect \
    libgtest-dev \
    libelf-dev \
    libsdl2-dev \
    liblua5.3-dev \
    libvncserver-dev \
    libslirp-dev \
    libusb-1.0-0-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
