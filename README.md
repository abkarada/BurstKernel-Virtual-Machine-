# NKernel

NKernel is an experimental, bare-metal unikernel/router designed as a fun side-project. Its original purpose was to overcome NAT Traversal issues by implementing the NATGhost algorithm without the overhead of standard operating system mechanics like context-switching and system calls. 

It runs directly on x86_64 hardware as an independent kernel, providing a fast-path NAT engine and a built-in CLI environment.

For the theoretical background of the NATGhost algorithm, please refer to the original reports:
- [NATGhost Algorithm Report (English MD)](NATGhost_Algorithm_Report.md)
- [NATGhost Algoritma Raporu (Orijinal DOCX)](NATGhost_Algoritma_Raporu.docx)

## Features
- **Bare-Metal:** Runs independently, without relying on Linux or any other host OS.
- **NAT Engine:** Performs fast IP and Port translation natively.
- **Built-in CLI & Telnet:** Includes a fully functional colored command-line interface, a virtual file system (VFS) in RAM, an integrated text editor (`edit`), and Telnet access on port 23.
- **Network Topology (`ntop`):** Visualizes connected LAN hosts directly from the NAT table in a simple ASCII format.
- **Raw Storage:** Saves configuration directly to raw sectors of an ATA disk without needing complex filesystems like FAT32 or EXT4.

## Running in QEMU (Testing)

To try it out in a virtual machine, you will need `gcc`, `make`, and `qemu-system-x86_64`.

```bash
make slirp
```

This will:
1. Compile the kernel (`nostdlib`).
2. Create a 1MB `config.img` virtual disk for storage.
3. Launch QEMU and map the Telnet port to `2323` on your host.

Once booted, type `help` to see the available commands. You can also connect remotely by running `telnet 127.0.0.1 2323` from your host machine.

## Installing on Real Hardware (Bare Metal)

NKernel is Multiboot compliant, which means it can be booted by standard bootloaders like GRUB.

1. **Compile the Kernel:**
   Run `make` to compile the source code into `kernel.elf`.

2. **Prepare a Bootable USB/Disk:**
   You can use GRUB to boot `kernel.elf`. A basic `grub.cfg` might look like this:
   ```
   menuentry "NKernel" {
       multiboot /boot/kernel.elf
       boot
   }
   ```
   
3. **Generate an ISO:**
   If you have `grub-mkrescue` and `xorriso` installed, you can generate a bootable ISO image:
   ```bash
   make iso
   ```
   This will produce `nkernel.iso` which can be flashed to a USB drive using tools like `dd` or Rufus.
   ```bash
   sudo dd if=nkernel.iso of=/dev/sdX bs=4M status=progress
   ```
   *(Replace `/dev/sdX` with your actual USB drive).*

4. **Storage Note:**
   If you intend to save configurations (`save` / `load`), ensure the system has an ATA IDE drive attached. The kernel's built-in ATA PIO driver currently expects this to read/write the raw configuration sectors.
