#include <stdio.h>
#include <string.h>

void sepreateIp(char input[1024]){
    char ip_address[16];
    char port[6];
    char *colon_pos = strchr(input,':');
    if(colon_pos!=NULL){
        size_t ip_len = colon_pos - input;
        strncpy(ip_address,input,ip_len);
        ip_address[ip_len] ='\0';
        printf("IP Address: %s\n", ip_address);
        printf("Port: %s\n", colon_pos+1);
    }

}
void main(){
    char input[1024] ="127.0.0.1:8000";
    sepreateIp(input);
}
