#ifndef EDIT_H
#define EDIT_H

void editor_start(const char *filename);
void editor_handle_key(char c);

extern int in_editor_mode;

#endif
