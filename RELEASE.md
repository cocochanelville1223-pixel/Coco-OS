# Coco OS Release Checklist

## ✅ Build verification
- [x] `make clean` completes without errors
- [x] `make` compiles 32-bit kernel (`build/kernel32.bin`) and 64-bit kernel (`build/kernel64.bin`)
- [x] `make` produces ISO files:
  - `cocoos-32bit.iso`
  - `cocoos-64bit.iso`
- [ ] If `grub-mkrescue` is missing, install it (Debian/Ubuntu: `sudo apt install grub2-common xorriso`)

## ✅ Quick test steps
- Run 64-bit in QEMU:
  - `qemu-system-x86_64 -cdrom cocoos-64bit.iso -m 1024 -smp 2`
  - (or) `qemu-system-i386 -cdrom cocoos-32bit.iso -m 1024 -smp 2`
- Press any key to start the GUI desktop when the kernel finishes booting.
- Test commands in shell mode:
  - `help`, `assistant`, `appstore`, `explorer`, `settings`, `taskmgr`, `reboot`
- Test desktop mode: Windows-like Start menu, app selection, pointer movement simulation.

## ✅ Documentation / user guide
- [x] `README.md` describes features, build/run commands, and usage tips.
- [x] `RELEASE.md` includes full release steps and known current constraints.

## 🔧 Known limitations (important to document before release)
- Core kernel is experimental and prototype only (no real preemptive scheduling, no filesystem support)
- Input is QEMU keyboard-only (mouse is simulated logic not hardware-driver integrated)
- In-OS app capabilities are stubs, not full production applications
- Networking and persistent storage are not implemented

## ⭐ Optional next release enhancements
- Implement real keyboard/mouse driver via PS/2 or USB in kernel
- Add FAT32 / ext2 driver and virtual disk image support
- Add preemption & process scheduler with IRQ timer
- Add real windowing/compositor with multiple application contexts
- Add persistence for settings and app installation state

## 📦 Release artifact packaging
- Tag the release e.g. `v0.1.0-coco-os`.
- Include `README.md`, `RELEASE.md`, `Makefile`, `boot/`, `kernel/` and generated `*.iso` in GitHub release assets.
- Provide automated script optionally:
```
make clean && make
qemu-system-x86_64 -cdrom cocoos-64bit.iso -m 2048 -smp 4
```

## 💡 Final release statement
You’re ready to publish as a prototype OS. Emphasize that this is a proof-of-concept educational operating system with CLI + framebuffer demo, not yet suitable for production deployment.
