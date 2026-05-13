#include <stdio.h>
#include <stdlib.h>
#include "datastruct.h"
#include "common/config.h"
#include "common/logging.h"
#include "clint/connction_cash.h"
#include "clint/lookup.h"
#include "clint/writefuncs.h"
#include "clint/read_file.h"

int main(int argc, char *argv[]){
    const char *config_path = (argc > 1) ? argv[1] : "config/dfs.conf";
    dfs_config_t config;
    if (dfs_config_load(config_path, &config) != 0) {
        fprintf(stderr, "Failed to load config: %s\n", dfs_config_get_error());
        return EXIT_FAILURE;
    }

    log_init(stdout);
    log_set_level(LOG_LEVEL_INFO);

    int option;
    printf("\nDistributed File System Client\n");
    printf("1) Lookup block\n2) Write block\n3) Read entire file\nSelect option: ");
    if (scanf("%d", &option) != 1) {
        fprintf(stderr, "Invalid option\n");
        return EXIT_FAILURE;
    }

    switch(option){
        case 1: lookupBlock(&config); break;
        case 2: writeFile(&config); break;
        case 3: readFile(&config); break;
        default: printf("No action selected\n");
    }

    log_shutdown();
    return 0;
}

