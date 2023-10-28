#include <vector>
#include <string>
#include <stdio.h>
#include <stdlib.h>
using namespace std;

typedef struct {
    char id[10];
    in_addr ip_address;
    in_port_t port;
    int socket;
    int connected;
} tcp_client;

typedef struct {
    int socket;
    vector<string> topics; // topicurile la care este conectat clientul de pe socket
} topics_struct;

typedef struct {
    string client_id;
    vector<pair<string, int>> topics_sf; // perechea (topic, sf)
} sf_struct;

typedef struct {
    in_addr ip_address;
    in_port_t port;
    char topic[50];
    uint8_t data_type;
    char payload[1500];
} udp_message;

typedef struct {
    char topic[50];
    uint8_t data_type;
    char payload[1500];
}udp_message_received;

#define DIE(assertion, call_description)	\
	do {									\
		if (assertion) {					\
			fprintf(stderr, "(%s, %d): ",	\
					__FILE__, __LINE__);	\
			perror(call_description);		\
			exit(EXIT_FAILURE);				\
		}									\
	} while(0)

#define BUFFER_LENGTH	1500
#define MAX_CLIENTS	100