#include "cli.h"
#include "libc.h"
#include "e1000.h"
#include "nat.h"
#include "kernel.h"

extern void puts(const char *s);
extern void puts_serial(const char *s);

void (*cli_puts)(const char *s) = puts;
void (*cli_set_color)(uint8_t fg, uint8_t bg) = set_color;

// We need access to route_entry to print routes
struct route_entry {
    uint32_t dest_ip;
    uint32_t netmask;
    uint8_t next_hop_mac[6];
};

#define MAX_ROUTES 16
extern struct route_entry routing_table[MAX_ROUTES];
extern int num_routes;

// Helper to print IP in x.x.x.x format (assuming Little Endian uint32_t)
static void print_ip(uint32_t ip) {
    uint8_t *p = (uint8_t *)&ip;
    char buf[16];
    // Simple custom itoa for IP parts
    for(int i=0; i<4; i++) {
        uint8_t v = p[i];
        if (v >= 100) {
            char str[2] = { '0' + (v / 100), '\0' };
            cli_puts(str);
            v %= 100;
            str[0] = '0' + (v / 10);
            cli_puts(str);
            v %= 10;
        } else if (v >= 10) {
            char str[2] = { '0' + (v / 10), '\0' };
            cli_puts(str);
            v %= 10;
        }
        char str[2] = { '0' + v, '\0' };
        cli_puts(str);
        if (i < 3) cli_puts(".");
    }
}

static void print_mac(const uint8_t *mac) {
    const char hex[] = "0123456789ABCDEF";
    for(int i=0; i<6; i++) {
        char str[3] = { hex[mac[i] >> 4], hex[mac[i] & 0x0F], '\0' };
        cli_puts(str);
        if (i < 5) cli_puts(":");
    }
}

void cli_execute(const char *cmd) {
    if (strcmp(cmd, "") == 0) {
        return; // Empty command
    }
    
    if (strncmp(cmd, "help", 4) == 0) {
        cli_set_color(VGA_YELLOW, VGA_BLACK);
        cli_puts("Available commands:\n");
        cli_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
        cli_puts("  help        ");
        cli_set_color(VGA_WHITE, VGA_BLACK);
        cli_puts("- Show this message\n");
        
        cli_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
        cli_puts("  ifconfig    ");
        cli_set_color(VGA_WHITE, VGA_BLACK);
        cli_puts("- List all E1000 NICs\n");
        
        cli_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
        cli_puts("  route print ");
        cli_set_color(VGA_WHITE, VGA_BLACK);
        cli_puts("- Show Fast-Path routing table\n");
        
        cli_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
        cli_puts("  info        ");
        cli_set_color(VGA_WHITE, VGA_BLACK);
        cli_puts("- Show system info\n");
    }
    else if (strncmp(cmd, "info", 4) == 0) {
        cli_set_color(VGA_LIGHT_MAGENTA, VGA_BLACK);
        cli_puts("NKernel (Network Kernel) - Unikernel Router\n");
        cli_set_color(VGA_LIGHT_BLUE, VGA_BLACK);
        cli_puts("Architecture: x86_64\n");
    }
    else if (strncmp(cmd, "ifconfig", 8) == 0) {
        if (num_e1000_devices == 0) {
            cli_set_color(VGA_LIGHT_RED, VGA_BLACK);
            cli_puts("No NICs found.\n");
        }
        for (int i = 0; i < num_e1000_devices; i++) {
            cli_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
            cli_puts("en"); 
            char id[2] = { '0' + i, '\0' };
            cli_puts(id);
            cli_set_color(VGA_WHITE, VGA_BLACK);
            cli_puts(": MAC=");
            cli_set_color(VGA_YELLOW, VGA_BLACK);
            print_mac(e1000_devs[i].mac);
            cli_set_color(VGA_DARK_GRAY, VGA_BLACK);
            cli_puts(" (E1000)\n");
        }
    }
    else if (strncmp(cmd, "route print", 11) == 0) {
        cli_set_color(VGA_YELLOW, VGA_BLACK);
        cli_puts("Destination     Netmask         Next-Hop MAC\n");
        cli_puts("--------------------------------------------------\n");
        for (int i = 0; i < num_routes; i++) {
            cli_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
            print_ip(routing_table[i].dest_ip);
            cli_puts("\t");
            cli_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
            print_ip(routing_table[i].netmask);
            cli_puts("\t");
            cli_set_color(VGA_LIGHT_MAGENTA, VGA_BLACK);
            print_mac(routing_table[i].next_hop_mac);
            cli_puts("\n");
        }
    }
    else if (strncmp(cmd, "save", 4) == 0) {
        // Save routing table to sector 1
        uint8_t buffer[512] = {0};
        extern void ata_write_sector(uint32_t lba, const uint8_t *buffer);
        // First 4 bytes: num_routes
        *(uint32_t *)buffer = num_routes;
        // Next bytes: routing_table array
        memcpy(buffer + 4, routing_table, sizeof(routing_table));
        
        ata_write_sector(1, buffer);
        cli_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
        cli_puts("Configuration saved to disk (ATA PIO Sector 1).\n");
    }
    else if (strncmp(cmd, "load", 4) == 0) {
        // Load routing table from sector 1
        uint8_t buffer[512] = {0};
        extern void ata_read_sector(uint32_t lba, uint8_t *buffer);
        
        ata_read_sector(1, buffer);
        num_routes = *(uint32_t *)buffer;
        if (num_routes > MAX_ROUTES) num_routes = 0; // sanity check
        
        memcpy(routing_table, buffer + 4, sizeof(routing_table));
        cli_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
        cli_puts("Configuration loaded from disk.\n");
    }
    else if (strncmp(cmd, "ls", 2) == 0) {
        // Virtual File System list
        cli_set_color(VGA_LIGHT_BLUE, VGA_BLACK);
        cli_puts("startup-config  ");
        cli_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
        cli_puts("running-config  ");
        cli_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
        cli_puts("version\n");
    }
    else if (strncmp(cmd, "exit", 4) == 0) {
        cli_set_color(VGA_YELLOW, VGA_BLACK);
        cli_puts("Halting system. Use Ctrl-A X to exit QEMU.\n");
        // We can halt the CPU:
        __asm__ volatile ("cli; hlt");
    }
    else if (strncmp(cmd, "clear", 5) == 0) {
        clear_screen();
    }
    else if (strncmp(cmd, "ntop", 4) == 0) {
        cli_set_color(VGA_WHITE, VGA_BLACK);
        cli_puts("      [ INTERNET ]\n");
        cli_puts("           |\n");
        cli_puts("           | (WAN: 10.0.2.15)\n");
        cli_set_color(VGA_LIGHT_MAGENTA, VGA_BLACK);
        cli_puts("   +---------------+\n");
        cli_puts("   | NKernel Router|\n");
        cli_puts("   +-------+-------+\n");
        cli_set_color(VGA_WHITE, VGA_BLACK);
        cli_puts("           | (LAN: 192.168.1.1)\n");
        cli_puts("           |\n");
        cli_puts("    +------+------+\n");
        
        // Scan NAT table for active hosts
        extern struct nat_entry nat_table[MAX_NAT_ENTRIES];
        uint32_t active_hosts[10];
        int host_count = 0;
        
        for (int i = 0; i < 2048; i++) { // MAX_NAT_ENTRIES
            if (nat_table[i].in_use) {
                int found = 0;
                for (int j = 0; j < host_count; j++) {
                    if (active_hosts[j] == nat_table[i].internal_ip) {
                        found = 1; break;
                    }
                }
                if (!found && host_count < 10) {
                    active_hosts[host_count++] = nat_table[i].internal_ip;
                }
            }
        }
        
        cli_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
        if (host_count == 0) {
            cli_puts("      [No Hosts]\n");
        } else {
            for (int i = 0; i < host_count; i++) {
                cli_puts("  [");
                print_ip(active_hosts[i]);
                cli_puts("] ");
            }
            cli_puts("\n");
        }
    }
    else if (strncmp(cmd, "cat ", 4) == 0) {
        const char *arg = cmd + 4;
        if (strcmp(arg, "version") == 0) {
            cli_set_color(VGA_LIGHT_MAGENTA, VGA_BLACK);
            cli_puts("NKernel v1.0 (Unikernel Router) - x86_64\n");
        }
        else if (strcmp(arg, "running-config") == 0) {
            cli_set_color(VGA_WHITE, VGA_BLACK);
            cli_puts("Current Routing Table (in RAM):\n");
            cli_execute("route print");
        }
        else if (strcmp(arg, "startup-config") == 0) {
            cli_set_color(VGA_WHITE, VGA_BLACK);
            cli_puts("Saved Routing Table (in ATA Disk):\n");
            uint8_t buffer[512] = {0};
            extern void ata_read_sector(uint32_t lba, uint8_t *buffer);
            ata_read_sector(1, buffer);
            uint32_t saved_num_routes = *(uint32_t *)buffer;
            if (saved_num_routes > MAX_ROUTES) saved_num_routes = 0;
            
            struct route_entry saved_routes[MAX_ROUTES];
            memcpy(saved_routes, buffer + 4, sizeof(saved_routes));
            
            cli_set_color(VGA_YELLOW, VGA_BLACK);
            cli_puts("Destination     Netmask         Next-Hop MAC\n");
            cli_puts("--------------------------------------------------\n");
            for (uint32_t i = 0; i < saved_num_routes; i++) {
                cli_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
                print_ip(saved_routes[i].dest_ip);
                cli_puts("\t");
                cli_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
                print_ip(saved_routes[i].netmask);
                cli_puts("\t");
                cli_set_color(VGA_LIGHT_MAGENTA, VGA_BLACK);
                print_mac(saved_routes[i].next_hop_mac);
                cli_puts("\n");
            }
        }
        else {
            cli_set_color(VGA_LIGHT_RED, VGA_BLACK);
            cli_puts("cat: ");
            cli_puts(arg);
            cli_puts(": No such file or directory\n");
        }
    }
    else {
        cli_set_color(VGA_LIGHT_RED, VGA_BLACK);
        cli_puts("Unknown command: ");
        cli_puts(cmd);
        cli_puts("\n");
    }
    cli_set_color(VGA_WHITE, VGA_BLACK); // Reset to default
}
