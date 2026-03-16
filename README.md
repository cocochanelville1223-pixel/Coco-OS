# Coco-OS
Coco OS is an educational experimental operating system that is now enriched with an AI assistant (Coco) plus App Store, File Explorer, and Settings features.

## What’s included
- `boot/boot.S`: Multiboot header + entry point into the kernel.
- `kernel/main.c`: VGA text-mode kernel with interactive shell.
- `kernel/kernel.h`: Kernel API declarations.
- `Makefile`: 32-bit build and ISO packaging for QEMU/GRUB.
- `.gitignore`: build artifacts ignored.

## Core features implemented
- Text-mode UI using VGA at `0xB8000`
- Command shell with commands:
  - `help`
  - `assistant`
  - `appstore`
  - `explorer`
  - `taskmgr`
  - `settings`
  - `terminal`
  - `windows-terminal`
  - `store`
  - `edge`
  - `outlook`
  - `teams`
  - `phonelink`
  - `onenote`
  - `text`
  - `clock`
  - `calc`
  - `media`
  - `photos`
  - `paint`
  - `clipchamp`
  - `notepad`
  - `security`
  - `copilot`
  - `sticky`
  - `voice`
  - `maps`
  - `weather`
  - `xbox`
  - `solitaire`
  - `youtube`
  - `clear`
  - `reboot`
- `Coco Assistant` greeting and assistant prompts
- App marketplace stub (Notes, Music, Paint, Terminal, Web, Calc, Chat, etc.)
- File Explorer stubs with virtual tree structure
- Task Manager stub showing processes and kill hint
- Settings stub with updates, wallpaper, personalization, network, users
- Productivity/apps stubs: browser, text editor, calculator, media player, photo viewer
- Communication stubs: mail+calendar, chat, snipping tool

## Build & Run (Linux)
1. Install dependencies:
   - Debian/Ubuntu: `sudo apt install build-essential qemu-system-x86 grub2-common xorriso`
2. `make clean && make`
3. Run ISO in QEMU:
   - 64-bit: `qemu-system-x86_64 -cdrom cocoos-64bit.iso -m 1024 -smp 2`
   - 32-bit: `qemu-system-i386 -cdrom cocoos-32bit.iso -m 1024 -smp 2`

## How to use
- `coco> assistant`: invoke Coco AI assistant
- `coco> appstore`: view and install apps
- `coco> explorer`: open virtual file explorer
- `coco> settings`: system settings menu
- `coco> help`: list commands

## Future enhancements
- Real filesystem + file I/O
- Kernel driver stack (storage, network, input, graphics)
- Preemptive scheduler and process isolation
- Real GUI shell with windowing and wallpapers
- Persistent update manager and package store backend
