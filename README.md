# 🔥 burstkernel - Minimal Microkernel for NATGhost UDP Burst

**burstkernel** is a lightweight microkernel designed for the [NATGhost](https://github.com/yourusername/natghost) project. Its sole purpose is to automatically send high-speed UDP bursts during system boot, without requiring any user interaction. This kernel is optimized for NAT traversal experiments and high-throughput packet injection.

---

## 🚀 Purpose

- Automatically launches on boot and sends UDP bursts.
- Scans PCI to detect the network interface (e.g., Intel E1000).
- Uses MMIO to directly control the NIC without an operating system.
- Sends UDP packets to a predefined target IP and port range.
- Ideal for NAT hole punching and symmetric NAT testing scenarios.
- Designed to run in thousands of parallel VMs with minimal overhead.

---

## 🧠 Architecture

├── boot/
│ └── boot.S # Multiboot-compliant bootloader stub
├── kernel/
│ ├── kernel.c # Main kernel logic and entry point
│ ├── pci.c # PCI scanner to locate the NIC
│ ├── e1000.c # MMIO-based Intel E1000 NIC driver
│ ├── burst.c # High-speed UDP packet blaster
│ └── config.h # Compile-time configuration (target IP/port)
├── build/ # Compiled object files
├── Makefile # Build instructions
└── README.md # Project documentation

🧪 Testing with QEMU

qemu-system-i386 -cdrom kernel.iso -nographic
Or, to simulate a real NIC environment:
qemu-system-i386 -cdrom kernel.iso -device e1000 -netdev user,id=n1 -device e1000,netdev=n1

Testing with VirtualBox
Create a new VM (32-bit, 64MB RAM is enough).

Set Network Adapter to Bridged Adapter or Host-only Adapter.

Attach kernel.iso to the virtual CD/DVD drive.

Boot the machine – packet blasting starts immediately.


Configuration
#define TARGET_IP   0xC0A80001  // 192.168.0.1
#define START_PORT  10000
#define END_PORT    20000

Then rebuild:
make clean && make all


Advanced Usage
This project is part of the NATGhost framework, which aims to:
Dynamically test NAT traversal capabilities.
Analyze port allocation randomness.
Perform coordinated bursts across thousands of VMs.

