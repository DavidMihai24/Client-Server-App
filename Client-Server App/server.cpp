#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <bits/stdc++.h>
#include "structs.h"

using namespace std;

// Functie care verifica daca un id al unui client sau un socket al unui client exista deja in vectorul de clienti
int check_existing (vector<tcp_client> &clients, tcp_client &current_client, int socket, char* field) {
    for (long unsigned int i = 0; i < clients.size(); i++) {
        if (strcmp(field, "socket") == 0) {
            if (clients[i].socket == socket) {
                return 0;
            }
        }
        if (strcmp(field, "id") == 0) {
            if (strcmp(clients[i].id, current_client.id) == 0) {
                return 0;
            }
        }   
    }
    return 1;
}

int main (int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    DIE(argc != 2, "Usage: ./server <PORT_DORIT>\n");

    fd_set readFds, tmpFds;
    vector<tcp_client> clients; // vector de clienti
    vector<sf_struct> sf_vector; // vector de store and forward
    vector<topics_struct> topics_vector; // vector de topic-uri
    vector<pair<udp_message, vector<string>>> messages; // vector de mesaje UDP si id-uri de clienti
    
    struct sockaddr_in tcp_client_addr;
    struct sockaddr_in udp_client_addr;

    FD_ZERO(&readFds);
    FD_ZERO(&tmpFds);

    // Initializam socket-ul UDP
    int udp_sockfd;
    struct sockaddr_in serv_udp_addr;
    DIE((udp_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0, "UDP socket error\n");
    memset(&serv_udp_addr, 0, sizeof(serv_udp_addr));
    serv_udp_addr.sin_addr.s_addr = INADDR_ANY;
    serv_udp_addr.sin_family = AF_INET;
    serv_udp_addr.sin_port = htons(atoi(argv[1]));
    DIE(bind(udp_sockfd, (const struct sockaddr *) &serv_udp_addr, sizeof(serv_udp_addr)) < 0, "Error binding UDP.\n");

    // Initializam socket-ul TCP
    int tcp_sockfd;
    struct sockaddr_in serv_tcp_addr;
    DIE((tcp_sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0, "TCP socket error\n");
    // Dezactivam algoritmul lui Nagle pentru socket-ul TCP
    int flag = 1;
    DIE(setsockopt(tcp_sockfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int)) < 0, "Error disabling Nagle's algorithm.\n");
    memset(&serv_tcp_addr, 0, sizeof(serv_tcp_addr));
    serv_tcp_addr.sin_addr.s_addr = INADDR_ANY;
    serv_tcp_addr.sin_family = AF_INET;
    serv_tcp_addr.sin_port = htons(atoi(argv[1]));
    DIE(bind(tcp_sockfd, (const struct sockaddr *) &serv_tcp_addr, sizeof(serv_tcp_addr)) < 0, "TCP binding error\n");
    DIE(listen(tcp_sockfd, MAX_CLIENTS) < 0, "Error listening to TCP.\n");

    FD_SET(udp_sockfd, &readFds);
    FD_SET(tcp_sockfd, &readFds);
    FD_SET(STDIN_FILENO, &readFds);
    
    int fdmax = 0;
    if (tcp_sockfd > udp_sockfd) {
        fdmax = tcp_sockfd;
    } else {
        fdmax = udp_sockfd;
    }

    for (;;) {
        tmpFds = readFds;

        DIE (select(fdmax + 1, &tmpFds, NULL, NULL, NULL) < 0, "Select error.\n");

        for (int i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &tmpFds)) {
                // Citirea de la tastatura
                if (i == STDIN_FILENO) {
                    char buffer[BUFFER_LENGTH];
                    memset(buffer, 0, BUFFER_LENGTH);
                    fgets(buffer, BUFFER_LENGTH - 1, stdin);

                    // Daca primim exit de la tastatura
                    if (strncmp(buffer, "exit", 4) == 0) {
                        for (long unsigned int i = 0; i < clients.size(); i++) {
                            DIE(send(clients[i].socket, "exit", sizeof(udp_message), 0) < 0, "Error sending exit message.\n");
                            printf("Client %s disconnected.\n", clients[i].id);
                        }
                        return 0;
                    } else {
                        fprintf(stderr, "%s", "Invalid command\n");
                        break;
                    }
                
                // Primim mesaj de la client UDP
                } else if (i == udp_sockfd) {
                    udp_message_received newmessage;
                    memset(&newmessage, 0, sizeof(udp_message_received));
                    socklen_t client_length = sizeof(udp_client_addr);

                    DIE((recvfrom(udp_sockfd, &newmessage, sizeof(udp_message_received), 0, (struct sockaddr *) &udp_client_addr,
                                    &client_length)) < 0, "Error receiving UDP message.\n");

                    udp_message new_message;
                    memset(&new_message, 0, sizeof(udp_message));

                    // Copiem datele din mesajul primit in mesajul pe care il trimitem clientilor
                    new_message.data_type = newmessage.data_type;
                    memcpy(new_message.topic, newmessage.topic, 50);
                    memcpy(new_message.payload, newmessage.payload, 1500);
                    new_message.ip_address = udp_client_addr.sin_addr;
                    new_message.port = ntohs(udp_client_addr.sin_port);

                    // Trimitem mesajul UDP catre clientii conectati
                    for (auto iterator = sf_vector.begin(); iterator != sf_vector.end(); iterator++) {
                        int ok = 1;
                        for (auto client_iterator = clients.begin(); client_iterator != clients.end(); client_iterator++) {
                            if (string(client_iterator->id) == iterator->client_id) {
                                for (auto topic_iterator = topics_vector.begin(); topic_iterator != topics_vector.end(); topic_iterator++) {
                                    if (topic_iterator->socket == client_iterator->socket) {
                                        vector<string> search_vector = topic_iterator->topics;
                                        ok = 0;
                                        if (find(search_vector.begin(), search_vector.end(), newmessage.topic) != search_vector.end()) {
                                            DIE(send(client_iterator->socket, &new_message, sizeof(udp_message), 0) < 0, "Error sending message.\n");
                                        }
                                        break;
                                    }
                                }
                            }
                        }

                        // Daca clientul nu este conectat si are s&f activat, retinem mesajul
                        if (ok == 1) {
                            vector<pair<string, int>> pairs_vector = iterator->topics_sf;
                            for (auto pairs_iterator = pairs_vector.begin(); pairs_iterator != pairs_vector.end(); pairs_iterator++) {
                                if (pairs_iterator->first == string(newmessage.topic)) {
                                    if (pairs_iterator->second == 1) {
                                        for (auto pairs_vector_iterator = messages.begin(); pairs_vector_iterator != messages.end(); pairs_vector_iterator++) {
                                            if (pairs_vector_iterator->first.data_type == new_message.data_type && strncmp(pairs_vector_iterator->first.topic, new_message.topic, 50) && strncmp(pairs_vector_iterator->first.payload, new_message.payload, 1500)) {
                                                ok = 0;
                                                pairs_vector_iterator->second.push_back(iterator->client_id);
                                            } 
                                        }

                                        // Daca nu am gasit mesajul in vectorul de mesaje, il adaugam
                                        if (ok == 1) {
                                            vector<string> new_entry;
                                            new_entry.push_back(iterator->client_id);
                                            messages.push_back(make_pair(new_message, new_entry));
                                        }
                                    }
                                }
                            }
                        }
                    }

                // Primim client nou TCP
                } else if (i == tcp_sockfd) {
                    socklen_t client_length = sizeof(tcp_client_addr);
                    int new_sockfd = accept(tcp_sockfd, (struct sockaddr *) &tcp_client_addr, &client_length);
                    DIE((new_sockfd < 0), "Accept error");

                    int flag = 1;
                    DIE(setsockopt(new_sockfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int)) < 0, "Error disabling Nagle's algorithm.\n");

                    FD_SET(new_sockfd, &readFds);

                    if (fdmax < new_sockfd) {
                        fdmax = new_sockfd;
                    }
                } else {
                    tcp_client auxClient;
                    if (check_existing(clients, auxClient, i, "socket") == 1) {
                        tcp_client newclient;
                        DIE(recv(i, &newclient, sizeof(tcp_client), 0) < 0, "Error receiving data.\n");
                        newclient.socket = i;

                        // Verificam daca clientul este deja conectat
                        if (check_existing(clients, newclient, 0, "id") == 0) {
                            printf("Client %s already connected.\n", newclient.id);
                            DIE(send(i, "exit", sizeof(udp_message), 0) < 0, "Error sending exit message.\n");
                            FD_CLR(i, &readFds);
                        } else {
                            printf("New client %s connected from %s:%hd\n", newclient.id, inet_ntoa(newclient.ip_address), newclient.port);
                            clients.push_back(newclient);

                            for (auto iterator = messages.begin(); iterator != messages.end(); iterator++) {
                                vector<string> clients_replacement;
                                for (auto iterator2 = iterator->second.begin(); iterator2 != iterator->second.end(); iterator2++) {
                                    if (strcmp((*iterator2).c_str(), newclient.id) == 0) {
                                        DIE(send(i, &iterator->first, sizeof(udp_message), 0) < 0, "Send error.\n");
                                    }
                                    else {
                                        clients_replacement.push_back(*iterator2);
                                    }
                                }
                                iterator->second = clients_replacement;
                            }
                        }

                    } else {
                        int buffer_length;
                        DIE(recv(i, &buffer_length, 4, 0) < 0, "Error receiving buffer length.\n");

                        char buffer[buffer_length];
                        DIE(recv(i, buffer, buffer_length, 0) < 0, "Error receiving buffer.\n");
                    
                        char *token = strtok(buffer, " ");

                        if (strncmp(token, "subscribe", 9) == 0) {
                            token = strtok(NULL, " ");
                            int ok = 1;
                            for (auto iterator = topics_vector.begin(); iterator != topics_vector.end(); iterator++) {
                                if (iterator->socket == i) {
                                    iterator->topics.push_back(token);
                                    ok = 0;
                                }
                            }
                            if (ok == 1) {
                                topics_struct newtopic;
                                newtopic.socket = i;
                                newtopic.topics.push_back(token);
                                topics_vector.push_back(newtopic);
                            }

                            int sf = atoi(token + strlen(token) + 1);
                            for (unsigned long int j = 0; j < clients.size(); j++) {
                                if (clients[j].socket == i) {
                                    int ok = 1;
                                    for (auto iterator = sf_vector.begin(); iterator != sf_vector.end(); iterator++) {
                                        if (iterator->client_id == string(clients[j].id)) {
                                            iterator->topics_sf.push_back(make_pair(token, sf));
                                            ok = 0;
                                        }
                                    }
                                    if (ok == 1) {
                                        sf_struct new_entry;
                                        new_entry.client_id = string(clients[j].id);
                                        new_entry.topics_sf.push_back(make_pair(token, sf));
                                        sf_vector.push_back(new_entry);
                                    }
                                }
                            }
                        } else if (strncmp(token, "unsubscribe", 11) == 0) {
                            token = strtok(NULL, " ");

                            for (auto iterator = topics_vector.begin(); iterator != topics_vector.end(); iterator++) {
                                if (iterator->socket == i) {
                                    for (auto iterator2 = iterator->topics.begin(); iterator2 != iterator->topics.end(); ) {
                                        if (strncmp((*iterator2).c_str(), token, strlen(token)) == 0) {
                                            iterator2 = iterator->topics.erase(iterator2);
                                        } else {
                                            iterator2++;
                                        }
                                    }
                                }
                            }

                            for (unsigned long int j = 0; j < clients.size(); j++) {
                                if (clients[j].socket == i) {
                                    for (auto iterator = sf_vector.begin(); iterator != sf_vector.end(); iterator++) {
                                        if (iterator->client_id == string(clients[j].id)) {
                                            for (auto iterator2 = iterator->topics_sf.begin(); iterator2 != iterator->topics_sf.end(); ) {
                                                if (strncmp((*iterator2).first.c_str(), token, strlen(token)) == 0) {
                                                    iterator2 = iterator->topics_sf.erase(iterator2);
                                                } else {
                                                    iterator2++;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        } else if (strncmp(token, "exit", 4) == 0) {
                            vector<tcp_client> clients_replacement;
                            for (auto client_iterator = clients.begin(); client_iterator != clients.end(); client_iterator++) {
                                if ((*client_iterator).socket == i) {
                                    printf("Client %s disconnected.\n", (*client_iterator).id);
                                    FD_CLR(i, &readFds);
                                    close(i);
                                    break;
                                }
                                else {
                                    clients_replacement.push_back(*client_iterator);
                                }
                            }
                            clients = clients_replacement;
                        }
                    }
                }
            }
        }
    }
    close(udp_sockfd);
    close(tcp_sockfd);
    return 0;
}