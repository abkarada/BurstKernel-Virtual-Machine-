# NKernel

NKernel is an experimental bare-metal x86 networking kernel written as a systems programming project.

It was originally created as part of a NAT traversal experiment called **NatGhost**, where the goal was to explore whether direct packet generation and NIC-level transmission could provide tighter control over large UDP packet bursts than a conventional userspace networking application.

The project later grew beyond that experiment into a broader exploration of kernel bootstrapping, device drivers, packet processing, routing, NAT, storage, and low-level network I/O.

NKernel is a research and learning project. It is not intended to be a production operating system, router, firewall, or NAT implementation.

## Overview

NKernel runs without a host operating system and provides a small environment for experimenting with networking directly on x86 hardware or through QEMU.

The current codebase includes:

- Multiboot-based kernel bootstrapping
- x86 interrupt setup
- basic memory initialization
- PCI device discovery
- an Intel E1000 network driver
- Ethernet packet transmission and reception
- lwIP integration
- basic IPv4 routing
- TCP/UDP NAT translation
- raw packet forwarding
- ATA PIO storage access
- a small command-line interface
- Telnet access
- a simple RAM-backed file environment
- a small text editor

The project is intentionally minimal and focuses on understanding how these components interact rather than providing a complete operating system.

## Architecture

A simplified view of the current system:

```text
                         ┌─────────────────────┐
                         │       GRUB          │
                         │     Multiboot       │
                         └──────────┬──────────┘
                                    │
                                    ▼
                         ┌─────────────────────┐
                         │    Kernel Entry     │
                         │      x86_64         │
                         └──────────┬──────────┘
                                    │
                  ┌─────────────────┼─────────────────┐
                  │                 │                 │
                  ▼                 ▼                 ▼
          ┌──────────────┐   ┌──────────────┐   ┌──────────────┐
          │     IDT      │   │    Memory    │   │    Serial    │
          │  Interrupts  │   │     Init     │   │   / VGA I/O  │
          └──────────────┘   └──────────────┘   └──────────────┘
                                    │
                                    ▼
                         ┌─────────────────────┐
                         │      PCI Scan       │
                         └──────────┬──────────┘
                                    │
                                    ▼
                         ┌─────────────────────┐
                         │   Intel E1000 NIC   │
                         │      Driver         │
                         └──────────┬──────────┘
                                    │
                                    ▼
                    ┌─────────────────────────────┐
                    │      Packet Processing      │
                    │                             │
                    │  Ethernet / IPv4 / NAT      │
                    │  Routing / lwIP integration │
                    └──────────────┬──────────────┘
                                   │
                    ┌──────────────┴───────────────┐
                    │                              │
                    ▼                              ▼
           ┌─────────────────┐            ┌─────────────────┐
           │   Local Stack   │            │ Packet Forward  │
           │      lwIP       │            │   / NAT Path    │
           └─────────────────┘            └─────────────────┘
```

## Boot and Kernel Environment

The kernel is booted through GRUB using the Multiboot format.

The early kernel environment initializes low-level components such as:

- VGA text output
- COM1 serial output
- interrupt handling
- memory initialization
- keyboard input
- network interfaces

The project avoids depending on Linux or another host operating system once the kernel is running.

## Intel E1000 Network Driver

NKernel includes a basic Intel E1000 network driver intended primarily for QEMU and compatible test environments.

The driver handles:

- PCI device discovery
- MMIO register access
- RX descriptor ring setup
- TX descriptor ring setup
- MAC address retrieval
- packet transmission
- packet reception

The implementation is experimental and makes simplifying assumptions about hardware and memory layout.

It should not be considered a general-purpose E1000 driver.

## Networking

NKernel uses two networking paths.

### Local protocol handling

Packets intended for the kernel itself can be passed to **lwIP**, which provides higher-level protocol support.

The current setup configures an IPv4 interface and uses lwIP for functionality such as ARP and local TCP/IP services.

### Direct packet path

Packets that are candidates for forwarding are processed directly by the kernel.

The forwarding path can:

- inspect Ethernet and IPv4 headers
- perform NAT translation
- decrement IPv4 TTL
- recompute the IPv4 checksum
- rewrite Ethernet addresses
- transmit the resulting frame through the E1000 driver

This path was originally introduced to experiment with packet processing without passing all forwarded traffic through a conventional userspace networking stack.

## NAT

The repository contains a small stateful NAT implementation for TCP and UDP.

Outbound packets can be mapped from an internal address and port to a kernel-managed external port.

Inbound packets are matched against the NAT table and translated back to the corresponding internal address and port.

The implementation also updates IPv4 and transport checksums when addresses or ports are modified.

The NAT subsystem is intentionally limited. It does not attempt to implement the complete behavior of a production NAT gateway or connection-tracking system.

## Routing

NKernel includes a small routing table used by the experimental forwarding path.

A route stores:

- destination network
- network mask
- next-hop MAC address

When a matching route is found, the kernel updates the packet and forwards it directly through the network interface.

The routing implementation is deliberately small and is primarily used to study forwarding mechanics.

## CLI and Remote Access

NKernel includes a small command-line environment for interacting with the running kernel.

The CLI was built to make testing low-level functionality easier without repeatedly rebuilding the kernel.

The repository also contains Telnet support, allowing the command environment to be accessed remotely when the network stack is running.

These interfaces are development tools rather than hardened administrative services.

## Storage

The kernel contains an ATA PIO driver for basic disk access.

Configuration data can be written directly to raw disk sectors without requiring a full filesystem.

This is intentionally simple and is mainly used to explore direct storage access from a bare-metal environment.

## Development Environment

The easiest way to run NKernel is through QEMU.

### Requirements

Typical development dependencies include:

```text
gcc
make
qemu-system-x86_64
grub
xorriso
```

Exact package names vary by distribution.

## Running with QEMU

The repository includes Makefile targets for building and launching the kernel.

For the QEMU networking setup:

```bash
make slirp
```

This builds the kernel and starts QEMU with the networking configuration used by the project.

After booting, the kernel CLI can be used directly through the VM console.

For the Telnet interface, the QEMU configuration exposes the guest service through a host port.

For example:

```bash
telnet 127.0.0.1 2323
```

## Building an ISO

If GRUB and `grub-mkrescue` are available:

```bash
make iso
```

This generates a bootable ISO containing the kernel.

The resulting image can be tested in QEMU or written to removable media for hardware experiments.

## Real Hardware

NKernel was primarily developed and tested as an experimental kernel environment.

Booting on real hardware is possible through GRUB/Multiboot, but hardware compatibility is limited by the drivers implemented in the repository.

In particular, networking currently depends on the supported E1000 path and storage functionality assumes ATA PIO-compatible hardware.

Use real-hardware booting only as an experimental setup.

## NatGhost Experiment

NKernel originally grew out of an earlier NAT traversal experiment called **NatGhost**.

The experiment explored an idea: whether bypassing a conventional userspace socket model and generating packets closer to the network device could make high-rate UDP port experiments easier to control.

That work was one of the motivations for implementing:

- direct packet construction
- direct E1000 transmission
- polling-based packet processing
- a small NAT implementation
- a minimal networking-focused kernel environment

The original experiment should be treated as exploratory work rather than as a claim that symmetric NAT traversal was generally solved.

Historical documents from that work are preserved in this repository for reference:

- [`NATGhost_Algorithm_Report.md`](NATGhost_Algorithm_Report.md)
- [`NATGhost_Algoritma_Raporu.docx`](NATGhost_Algoritma_Raporu.docx)

Some statements in those documents reflect the assumptions and conclusions of the experiment at the time they were written. They are retained as historical design material and should not be interpreted as current claims about NAT traversal or ICE behavior.

## Project Scope

NKernel is mainly an exercise in understanding the boundary between software and hardware.

The project touches several areas:

- kernel bootstrapping
- interrupt handling
- memory management
- PCI enumeration
- device drivers
- DMA-oriented NIC interfaces
- Ethernet
- IPv4
- NAT
- routing
- packet checksums
- embedded TCP/IP stacks
- raw disk access
- polling-based I/O

Many components are intentionally simplified.

The goal of the project is not to compete with Linux, BSD, DPDK, production router software, or mature unikernel projects. It exists as a compact environment for experimenting with low-level systems and networking concepts.

## Limitations

Current limitations include:

- limited hardware support
- experimental memory and DMA assumptions
- simplified routing
- simplified NAT state management
- no production-grade firewall
- no general-purpose scheduler
- no process model
- no userspace isolation
- limited filesystem functionality
- development-oriented Telnet interface
- incomplete protocol coverage
- limited hardware validation

## Repository Layout

Some of the main source files include:

```text
boot.s
    Early boot and architecture setup.

kernel.c
    Kernel initialization and the main packet-processing path.

idt.c
    Interrupt Descriptor Table setup.

e1000.c
    Intel E1000 network driver.

nat.c
    Experimental TCP/UDP NAT implementation.

ata.c
    ATA PIO disk access.

cli.c
    Built-in command-line environment.

edit.c
    Small text editor used from the kernel CLI.

grub.cfg
    GRUB configuration used for booting the kernel.
```

Additional networking and support code is included elsewhere in the repository.

## Status

NKernel is an experimental side project and is not under active production development.

The repository is kept public as a record of low-level systems and networking experiments.

## License

See [`LICENCE`](LICENCE) for license information.
