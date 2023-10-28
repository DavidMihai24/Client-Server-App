GHELESEL DAVID-MIHAI

In cadrul acestui proiect am implementat o aplicatie Client-Server care se ocupa cu gestionarea a diferite mesaje.

Fisiere folosite:
- structs.h
- server.cpp
- subscriber.cpp
- Makefile

In structs.h am definit urmatoarele structuri: 
- tcp_client - contine datele unui client;
- topics_struct - contine un vector de topic-uri la care este conectat un client si socket-ul respectivului client;
- sf_struct - contine id-ul unui client si un vector de perechi (topic, sf)
- udp_message - contine datele unui mesaj trimis de server
- udp_message_received - structura corespunzatoare unui mesaj primit de server de la un client UDP

In fisierul subscriber.cpp, instantiez structura unui client folosind campurile primite ca argumente din linia de comanda si un socket TCP, dupa care conectez clientul la server si ii trimit acestuia datele. Dupa aceea, intru intr-o bucla infinita in care avem 2 variante, sa primim un mesaj de la tastatura sau de la server. De la tastatura putem primi comenzile "exit", "subscribe" sau "unsubscribe". In primul caz, deconectez clientul de la server si opresc programul, iar in celelalte cazuri, trimit server-ului mesajele cu topic-urile de la care clientul doreste sa se (dez)aboneze. Daca primim mesajul de la server si daca comanda nu este "exit", afisez mesajul in functie de tipul sau de date. La final, inchid conexiunea.

In fisierul server.cpp, am folosit urmatoarele structuri: un vector de clienti, un vector de structura sf_struct, un vector de topic-uri si un vector in care retin mesajele UDP. Mai intai am instantiat socket-urile TCP si UDP, dupa care intru din nou intr-o bucla infinita. Singura comanda posibila primita de la tastatura este "exit", caz in care deconectez serverul, toti clientii si opresc programul. Daca am primit de pe socket-ul UDP, trimitem mesajul mai departe tuturor clientilor TCP care sunt abonati la topic-ul respectiv. Daca am primit de pe socket-ul TCP, acceptam clientul care vrea sa se conecteze. Daca nu avem niciunul din aceste cazuri, inseamna ca am primit mesaj de la un client TCP care este deja conectat la server. Verific daca clientul este prezent in vectorul de clienti sau daca nu exista deja un alt client cu ID-ul lui, caz in care nu il accept. La fiecare (re)conectare a unui client, am verificat daca are mesaje din vectorul messages de primit, caz in care i le trimit. Daca clientul ne transmite comanda "subscribe", updatez vectorii de sf_struct si topics_struct cu perechea (socket, topic) si respectiv (id, (topic, sf)). Daca primim comanda "unsubscribe" sterg aceleasi perechi din vectorii lor respectivi. In sfarsit, daca primim "exit", deconectez clientul, il sterg din vectorul de clienti si inchid socket-ul.