#include <stdio.h>
#include <string.h>

#define MAX_BLOCKS 10
#define MAX_FILENAME_LEN 50
#define MAX_ADDR_LEN 50
#define MAX_DS 50


typedef struct {
    int blockid;
    char locations[MAX_ADDR_LEN][MAX_DS];
    int ports[MAX_DS];
} blockinfo;

typedef struct {
    char filename[MAX_FILENAME_LEN];
    int total_blocks;
    blockinfo blocks[MAX_BLOCKS];
} FileMap;

int main(){
    FILE *metadata;
    char *buffer;
    FileMap out;
    metadata=fopen("../build/metadata.txt","r");
    
    if (!metadata) {
        perror("Meta Data file not found!!");
        return 1;
    }
    char input[100] ="my_file.txt";
    
    while(fgets(buffer , sizeof(buffer), metadata)){
        printf("%s",buffer);
        if(strcmp(buffer, input)==0){
            out.filename = buffer
        }else
            continue;

    }

    return 0;
}
