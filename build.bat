@echo off
docker run --rm -it -v "%cd%:/workspace" -w /workspace nola-build make clean
docker run --rm -it -v "%cd%:/workspace" -w /workspace nola-build make CC=gcc
REM Обычный запуск. Для 1920x1080: раскомментируйте строку ниже (нужен QEMU с GTK)
REM "C:\Program Files\qemu\qemu-system-x86_64.exe" -cdrom kernel.iso -display gtk,zoom-to-fit=on
"C:\Program Files\qemu\qemu-system-x86_64.exe" -cdrom kernel.iso
