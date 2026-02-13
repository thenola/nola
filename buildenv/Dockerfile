FROM ubuntu:24.04

LABEL maintainer="nola-kernel"

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        build-essential \
        nasm \
        qemu-system-x86 \
        grub-pc-bin \
        xorriso \
        ca-certificates \
        make \
        gcc \
        git \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

# По умолчанию просто открываем shell, чтобы можно было запускать make.
CMD ["/bin/bash"]
