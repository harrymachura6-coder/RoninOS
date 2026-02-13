FROM randomdude/gcc-cross-x86_64-elf:latest

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
       make \
       xorriso \
       grub-pc-bin \
       grub-common \
       qemu-system-x86 \
       ca-certificates \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
