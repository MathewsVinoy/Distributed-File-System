#ifndef WRITEBLOCK_H
#define WRITEBLOCK_H

#include "datastruct.h"


int* write_block(FileMap datamap, int blockid);
void status_listen(int *ds_lists, int blockid);

#endif
