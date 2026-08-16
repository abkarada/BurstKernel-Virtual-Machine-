#include "vfs.h"
#include "libc.h"
#include "kernel.h"
#include "cli.h"

struct vfs_file vfs_nodes[MAX_VFS_FILES];

void vfs_init(void) {
    for (int i = 0; i < MAX_VFS_FILES; i++) {
        vfs_nodes[i].is_used = 0;
        vfs_nodes[i].size = 0;
    }
    
    // Create some default files
    struct vfs_file *f1 = vfs_create("version");
    if (f1) {
        strcpy(f1->data, "NKernel v1.0 (Unikernel Router) - x86_64\n");
        f1->size = strlen(f1->data);
    }
    
    struct vfs_file *f2 = vfs_create("readme.txt");
    if (f2) {
        strcpy(f2->data, "Welcome to NKernel VFS!\nThis file is stored in RAM.\nUse 'edit readme.txt' to modify it.\n");
        f2->size = strlen(f2->data);
    }
}

struct vfs_file* vfs_open(const char* name) {
    for (int i = 0; i < MAX_VFS_FILES; i++) {
        if (vfs_nodes[i].is_used && strcmp(vfs_nodes[i].name, name) == 0) {
            return &vfs_nodes[i];
        }
    }
    return NULL; // Not found
}

struct vfs_file* vfs_create(const char* name) {
    // Check if it already exists
    struct vfs_file* existing = vfs_open(name);
    if (existing) return existing;
    
    // Find free slot
    for (int i = 0; i < MAX_VFS_FILES; i++) {
        if (!vfs_nodes[i].is_used) {
            vfs_nodes[i].is_used = 1;
            
            // Manual strncpy to avoid missing libc function
            int j = 0;
            while (name[j] != '\0' && j < MAX_FILE_NAME - 1) {
                vfs_nodes[i].name[j] = name[j];
                j++;
            }
            vfs_nodes[i].name[j] = '\0';

            vfs_nodes[i].size = 0;
            vfs_nodes[i].data[0] = '\0';
            return &vfs_nodes[i];
        }
    }
    return NULL; // No space
}

void vfs_list(void) {
    extern void (*cli_puts)(const char *s);
    extern void (*cli_set_color)(uint8_t fg, uint8_t bg);
    
    int count = 0;
    for (int i = 0; i < MAX_VFS_FILES; i++) {
        if (vfs_nodes[i].is_used) {
            cli_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
            cli_puts(vfs_nodes[i].name);
            cli_puts("  ");
            count++;
        }
    }
    if (count > 0) {
        cli_puts("\n");
    } else {
        cli_set_color(VGA_LIGHT_RED, VGA_BLACK);
        cli_puts("No files found.\n");
    }
    cli_set_color(VGA_WHITE, VGA_BLACK);
}
