GHELESEL DAVID-MIHAI

In this project I implemented a Client-Server application that deals with the management of different messages.

Files used:

structs.h
server.cpp
subscriber.cpp
Makefile
In structs.h I defined the following structures:

tcp_client - contains the data of a client;
topics_struct - contains an array of topics to which a client is connected and the socket of that client;
sf_struct - contains the id of a client and a vector of pairs (topic, sf);
udp_message - contains the data of a message sent by the server;
udp_message_received - structure corresponding to a message received by the server from a UDP client.

In "subscriber.cpp", I instantiate the structure of a client using the fields received as command line arguments and a TCP socket, then connect the client to the server and send the data. After that, I enter an endless loop in which we have 2 choices: receive a message from the keyboard or from the server. From the keyboard we can receive the commands "exit", "subscribe" or "unsubscribe". In the first case, I disconnect the client from the server and stop the program, and in the other cases, I send the server the messages with the topics the client wants to (un)subscribe from. If we receive the message from the server and if the command is not "exit", I display the message according to its data type. Finally, I close the connection.

In the "server.cpp" file, I used the following structures: a vector of clients, a vector of sf_struct, a vector of topics and a vector in which I contain UDP messages. First I instantiated the TCP and UDP sockets, then again I enter an endless loop. The only possible command received from the keyboard is "exit", in which case I disconnect the server, all clients and stop the program. If I received it on the UDP socket, I send the message onward to all TCP clients that are subscribed to the topic. If we received from the TCP socket, we accept the client that wants to connect. If we don't have any of these cases, it means that we received the message from a TCP client that is already connected to the server. I check if the client is present in the client vector or if there already exists another client with its ID, in which case I do not accept it. At each (re)connection of a client, I check if it has messages in the message vector to receive, in which case I send them to it. If the client sends us the "subscribe" command, I update the sf_struct and topics_struct vectors with the pair (socket, topic) and (id, (topic, sf)) respectively. If we receive the "unsubscribe" command I delete those same pairs from their respective vectors. Finally, if we get "exit", I disconnect the client, delete it from the client vector and close the socket.
