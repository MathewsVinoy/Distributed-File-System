#ifndef LOAD_METADATA_H
#define LOAD_METADATA_H

#include "datastruct.h"

// Load metadata for the requested file into the provided FileMap.
// Returns 0 on success, -1 if the file is missing or metadata cannot be read.
int findlocation(const char *metadata_path, const char *input, FileMap *out);

#endif // LOAD_METADATA_H

