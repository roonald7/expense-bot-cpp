FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive
ENV VCPKG_ROOT=/opt/vcpkg
ENV PATH="${VCPKG_ROOT}:${PATH}"

# 1. Install build tools
RUN apt-get update && apt-get install -y \
    build-essential cmake git curl zip unzip tar pkg-config libssl-dev \
    && rm -rf /var/lib/apt/lists/*

# 2. Install vcpkg
WORKDIR /opt
RUN git clone https://github.com/microsoft/vcpkg.git && \
    ./vcpkg/bootstrap-vcpkg.sh

# 3. Pre-compile dependencies (this is the slow part)
WORKDIR /app
COPY vcpkg.json .
RUN vcpkg install

# Now this image has everything ready to compile ExpenseBot
