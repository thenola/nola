## Nola 64-bit kernel 

### build

```bash
@echo off
docker run --rm -it -v "%cd%:/workspace" -w /workspace nola-build make clean
docker run --rm -it -v "%cd%:/workspace" -w /workspace nola-build make CC=gcc
"C:\Program Files\qemu\qemu-system-x86_64.exe" -cdrom kernel.iso

```