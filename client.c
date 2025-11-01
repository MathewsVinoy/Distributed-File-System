#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>

typedef struct __attribute__((packed)) {
    int blockid;
    char locations[50][50];
    int ports[50];
} block;

int main() {
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(9000);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("Metadata server connection failed");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    char comment[100] = "WRITE_BLOCK my_file.txt BLOCK1";
    send(client_socket, comment, strlen(comment), 0);

    block location;
    ssize_t recv_bytes = recv(client_socket, &location, sizeof(location), 0);
    if (recv_bytes != sizeof(location)) {
        perror("Failed to receive full block structure");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    close(client_socket);

    // Debug: Print the received block structure
    printf("Block ID: %d\n", location.blockid);
    for (int i = 0; i < 50; i++) {
        if (strlen(location.locations[i]) > 0) {
            printf("Location %d: %s:%d\n", i, location.locations[i], location.ports[i]);
        }
    }

    char test[1024] = "\ntest Data writing to file";
    size_t test_len = strlen(test);

    // Connect to the data server
    for (int i = 0; i < 50; i++) {
        if (strlen(location.locations[i]) == 0) {
            break; // No more locations
        }

        printf("Attempting to connect to %s:%d\n", location.locations[i], location.ports[i]);

        client_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (client_socket == -1) {
            perror("Socket creation failed");
            continue;
        }

        server_addr.sin_port = htons(location.ports[i]);
        server_addr.sin_addr.s_addr = inet_addr(location.locations[i]);

        if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
            perror("Data server connection failed");
            close(client_socket);
            continue;
        }

        char request[50] = "PUT BLOCK 1";
        send(client_socket, request, strlen(request), 0);

        char buffer[1024] = {0};
        recv_bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (recv_bytes <= 0) {
            perror("Failed to receive response");
            close(client_socket);
            continue;
        }
        buffer[recv_bytes] = '\0'; // Null-terminate

        if (strcmp(buffer, "ok") == 0) {
            send(client_socket, test, test_len, 0);
            memset(buffer, 0, sizeof(buffer));
            recv_bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
            if (recv_bytes <= 0) {
                perror("Failed to receive confirmation");
                close(client_socket);
                continue;
            }
            buffer[recv_bytes] = '\0';
            if (strcmp(buffer, "ok") == 0) {
                printf("Data written successfully\n");
            } else {
                perror("Error occurred when writing");
            }
        } else {
            perror("Failed to write the data");
        }

        close(client_socket);
    }

    return 0;
}

