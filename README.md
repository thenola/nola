## Nola 64-bit kernel 
<img width="843" height="569" alt="{06DC130C-0EF4-4774-A45E-4A2943527AB8}" src="https://github.com/user-attachments/assets/ce7be6d9-6bd2-4a10-8176-e597214fb2f6" />

### build

```bash
@echo off
docker run --rm -it -v "%cd%:/workspace" -w /workspace nola-build make clean
docker run --rm -it -v "%cd%:/workspace" -w /workspace nola-build make CC=gcc
"C:\Program Files\qemu\qemu-system-x86_64.exe" -cdrom kernel.iso

```
