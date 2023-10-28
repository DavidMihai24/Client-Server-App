#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <math.h>
#include "structs.h"

int main (int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    DIE(argc != 4, "Usage: ./subscriber <ID_CLIENT> <IP_SERVER> <PORT_SERVER>\n");

    tcp_client new_tcp_client;
    memset(&new_tcp_client, 0, sizeof(tcp_client));
    memcpy(new_tcp_client.id, argv[1], strlen(argv[1]));
    DIE(inet_aton(argv[2], &new_tcp_client.ip_address) == 0, "Inet error");
    new_tcp_client.port = htons(atoi(argv[3]));

    int sockfd;
    int ret;
    struct sockaddr_in serv_addr;
    int fdmax;
    char buffer[BUFFER_LENGTH];

    fd_set readFds, tmpFds;

    FD_ZERO(&readFds);
    FD_ZERO(&tmpFds);

    // Initializam socket-ul TCP
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "TCP socket error\n");

    // Dezactivam algoritmul lui Nagle pentru socket-ul TCP
    int flag = 1;
    DIE(setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int)) < 0, "Error disabling Nagle's algorithm\n");

    // Avem 2 variante: citirea de la tastatura sau citirea de pe socketul TCP
    FD_SET(sockfd, &readFds);
    FD_SET(STDIN_FILENO, &readFds);
    fdmax = sockfd;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[3]));
    DIE(inet_aton(argv[2], &serv_addr.sin_addr) == 0, "Inet error");

    // Conectam clientul la server
    ret = connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
    DIE(ret < 0, "Error connecting\n");

    // Trimitem datele clientului la server
    DIE(send(sockfd, &new_tcp_client, sizeof(tcp_client), 0) < 0, "Error sending client information\n");
    for (;;) {
        tmpFds = readFds;

        //Citirea de la tastatura
        DIE(select(sockfd + 1, &tmpFds, NULL, NULL, NULL) < 0, "Select error\n");
        if (FD_ISSET(STDIN_FILENO, &tmpFds)) {
            memset(buffer, 0, BUFFER_LENGTH);
            fgets(buffer, BUFFER_LENGTH - 1, stdin);

            short buffer_length = strlen(buffer);

            // Daca primim comanda exit
            if (strncmp(buffer, "exit", 4) == 0) {
                DIE(send(sockfd, &buffer_length, sizeof(int), 0) < 0, "Error sending length.\n");
                DIE(send(sockfd, &buffer, buffer_length, 0) < 0, "Subscribe error.\n");
                break;
            }

            // Daca vrem sa ne abonam sau sa ne dezabonam de la un topic
            if (strncmp(buffer, "subscribe", 9) == 0) {
                DIE(send(sockfd, &buffer_length, sizeof(int), 0) < 0, "Error sending length.\n");
                DIE(send(sockfd, &buffer, buffer_length, 0) < 0, "Subscribe error.\n");
                printf("%s\n", "Subscribed to topic.");

            } else if (strncmp(buffer, "unsubscribe", 11) == 0) {
                DIE((send(sockfd, &buffer_length, sizeof(int), 0)) < 0, "Error sending length.\n");
                DIE((send(sockfd, &buffer, buffer_length, 0) < 0), "Unsubscribe error.\n");
                printf("%s\n", "Unsubscribed from topic.");

            } else {
                fprintf(stderr, "%s", "Unknown command\n");
                continue;
            }
        }

        // Daca primim de la server
        if (FD_ISSET(sockfd, &tmpFds)) {
            udp_message new_message;
            memset(&new_message, 0, sizeof(udp_message));
            DIE((recv(sockfd, &new_message, sizeof(udp_message), 0) < 0), "Message receive error");

            // Daca primim comanda exit
            if (strncmp(new_message.payload, "exit", 4) == 0) {
                break;
            }

            // Gestionam afisarea mesajelor in functie de tipul de date primit
            switch (new_message.data_type) {
                case 0: {
                    char *sign = new_message.payload;
                    if (*sign == 1) {
                        printf("%s:%hu - %s - %s - -%d\n", inet_ntoa(new_message.ip_address), new_message.port, 
                        new_message.topic, "INT", htonl(* (uint32_t *)(new_message.payload + 1)));
                    } else {
                        printf("%s:%hu - %s - %s - %d\n", inet_ntoa(new_message.ip_address), new_message.port, 
                        new_message.topic, "INT", htonl(* (uint32_t *)(new_message.payload + 1)));
                    }
                    break;
                }
                case 1: {
                    printf("%s:%hu - %s - %s - %.2f\n", inet_ntoa(new_message.ip_address), new_message.port, 
                    new_message.topic, "SHORT_REAL", (float) htons(* (uint16_t *)new_message.payload) / 100);
                    break;
                }
                case 2: {
                    char *sign = new_message.payload;
                    if (*sign == 0) {
                        printf("%s:%hu - %s - %s - %f\n", inet_ntoa(new_message.ip_address), new_message.port, 
                        new_message.topic, "FLOAT", htonl(*(uint32_t *)(new_message.payload + 1)) * pow(10, -(*(uint8_t *)(new_message.payload + 5))));
                    } else if (*sign == 1) {
                        printf("%s:%hu - %s - %s - %f\n", inet_ntoa(new_message.ip_address), new_message.port, 
                        new_message.topic, "FLOAT", -(htonl(*(uint32_t *)(new_message.payload + 1)) * pow(10, -(*(uint8_t *)(new_message.payload + 5)))));
                    }
                    break;
                } case 3: {
                    printf("%s:%hu - %s - %s - %s\n", inet_ntoa(new_message.ip_address), new_message.port, 
                    new_message.topic, "STRING", new_message.payload);
                    break;
                } default: {
                    fprintf(stderr, "%s\n", "Invalid data type.\n");
                    exit(1);
                }
            }
        }
    }
    // Inchidem socket-ul
    close(sockfd);
    return 0;
}