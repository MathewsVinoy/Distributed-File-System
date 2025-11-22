#include <stdio.h>
#include "datastruct.h"
#include "clint/connction_cash.h"
#include "clint/lookup.h"
#include "clint/writefuncs.h"
#include "clint/read_file.h"

int main(){
    int option;
    printf("\n\t\t\tEnter the operatrion that need to be performed: ");
    printf("\n1 for displaying block\n2 for write to the file\n 3 for  showing the full file");
    scanf("%d",&option);
    switch(option){
        case 1: lookupBlock(); break;
        case 2: writeFile(); break;
        case 3: readFile(); break;
        default: return 0;
    }
    return 0;
}

