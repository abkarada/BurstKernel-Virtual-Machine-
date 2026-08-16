#ifndef VFS_H
#define VFS_H

#define MAX_VFS_FILES 10
#define MAX_FILE_NAME 32
#define MAX_FILE_SIZE 4096

struct vfs_file {
    char name[MAX_FILE_NAME];
    char data[MAX_FILE_SIZE];
    int size;
    int is_used;
};

extern struct vfs_file vfs_nodes[MAX_VFS_FILES];

void vfs_init(void);
struct vfs_file* vfs_open(const char* name);
struct vfs_file* vfs_create(const char* name);
void vfs_list(void);

#endif
