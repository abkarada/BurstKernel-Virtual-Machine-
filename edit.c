#include "edit.h"
#include "vfs.h"
#include "kernel.h"
#include "libc.h"
#include "cli.h"

int in_editor_mode = 0;
static struct vfs_file *current_file = NULL;

extern void (*cli_puts)(const char *s);
extern void (*cli_set_color)(uint8_t fg, uint8_t bg);
extern void clear_screen(void);
extern void shell_execute(void); // To print prompt on exit

static void editor_redraw(void) {
    clear_screen();
    cli_set_color(VGA_BLACK, VGA_LIGHT_CYAN); // Inverted top bar
    cli_puts(" NKernel Editor v1.0 | File: ");
    cli_puts(current_file->name);
    cli_puts(" | Press ESC to save and exit.\n");
    cli_set_color(VGA_WHITE, VGA_BLACK);
    
    // Print file content
    char tmp[2] = {0, 0};
    for (int i = 0; i < current_file->size; i++) {
        tmp[0] = current_file->data[i];
        cli_puts(tmp);
    }
}

void editor_start(const char *filename) {
    current_file = vfs_create(filename);
    if (!current_file) {
        cli_set_color(VGA_LIGHT_RED, VGA_BLACK);
        cli_puts("Error: Cannot create or open file.\n");
        return;
    }
    
    in_editor_mode = 1;
    editor_redraw();
}

void editor_handle_key(char c) {
    if (c == 27) { // ESC key
        in_editor_mode = 0;
        clear_screen();
        cli_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
        cli_puts("File saved.\n");
        shell_execute(); // Print prompt again
        return;
    }
    
    if (c == '\b' || c == 127) { // Backspace
        if (current_file->size > 0) {
            current_file->size--;
            current_file->data[current_file->size] = '\0';
            editor_redraw();
        }
    } else {
        if (current_file->size < MAX_FILE_SIZE - 1) {
            current_file->data[current_file->size++] = c;
            current_file->data[current_file->size] = '\0';
            char tmp[2] = {c, '\0'};
            cli_puts(tmp); // Just print the character directly
        }
    }
}
