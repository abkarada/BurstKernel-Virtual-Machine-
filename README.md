# NKernel (Network Kernel) 🚀

*A simple, lightweight, and fun side-project exploring unikernel architecture for high-speed packet routing.*

## The Story: From BurstKernel to NKernel
This project started as **BurstKernel**, a Linux kernel module born out of a very specific networking necessity: **NAT Traversal**.

As detailed in the [NATGhost Algorithm Report](NATGhost_Algorithm_Report.md), traversing Double-Symmetric NATs without a relay (TURN server) requires aggressive port prediction and UDP hole punching. To achieve a high probability of success (rather than a mere 52%), a peer needs to open over 10,000 sockets and send packets almost simultaneously. 

However, doing this through standard Operating System syscalls (`socket()`, `bind()`, `sendto()`) incurs massive overhead. By the time the 10,000th packet is sent via the Linux kernel, the stateful firewall rules for the first packets have already timed out (firewalls typically give you just a few seconds). 

To solve this, we needed to bypass the OS completely. We needed to blast packets at the physical limit of the Gigabit link (no syscalls, no interrupts, no context switching). That was the goal of **BurstKernel**.

As the project evolved, we realized this architecture was essentially a minimal Unikernel. We decided to transition it into **NKernel**—a standalone, bare-metal router. It strips away file systems, POSIX compliance, and standard libraries to focus purely on one thing: **moving packets extremely fast.**

*Note: This is purely a fun, experimental side-project. It's an exploration of how OS kernels and network stacks work under the hood.*

## Features
- **No Underlying OS:** Boots directly on bare metal or hypervisors (QEMU/KVM/Proxmox).
- **Zero-Copy Routing:** Bypasses standard OS network stacks.
- **Virtual File System (VFS):** No FAT32 or EXT4. Configurations are written directly to raw ATA disk sectors.
- **Built-in CLI:** A colorful, Zsh-like command line interface (Omarchy style).
- **Remote Management (Telnet):** Manage the router remotely via Port 23.

## How to Run (QEMU)

If you have `gcc`, `make`, and `qemu-system-x86_64` installed on your Linux/macOS machine, running NKernel is incredibly simple:

```bash
make slirp
```

This command will:
1. Compile the kernel (`nostdlib`, freestanding).
2. Create a 1MB virtual hard disk (`config.img`) for raw sector storage if it doesn't exist.
3. Launch QEMU with a graphical VGA window.
4. Set up two E1000 Gigabit network interfaces.
5. Forward host port `2323` to guest port `23` for Telnet access.

### Accessing the CLI Remotely
While QEMU is running, you can connect to the router from your host machine:
```bash
telnet 127.0.0.1 2323
```

### Basic Commands
Once the CLI is open (either via QEMU's VGA or Telnet), you can try:
- `help`: Lists available commands.
- `ifconfig`: Displays the NICs, MAC addresses, and Link Status.
- `route print`: Displays the fast-path routing table.
- `ls`: Lists the virtual files (e.g., `running-config`, `startup-config`).
- `cat startup-config`: Reads the routing table saved directly on the ATA disk.
- `save`: Writes your current configuration to the raw ATA disk sectors.
- `load`: Loads the configuration from the disk.

## Architecture
- **Language:** C (GNU C99) and x86 Assembly.
- **Target:** x86_64 (64-bit Long Mode).
- **Network Stack:** A highly optimized polling driver for Intel E1000 NICs, integrated with `lwIP` (Lightweight IP) for TCP/Telnet support.
- **Storage:** Custom ATA PIO driver reading/writing raw sectors.

## License
MIT License. Feel free to explore, break, and learn from it!
