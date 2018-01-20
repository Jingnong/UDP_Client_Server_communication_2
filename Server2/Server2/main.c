//
//  main.c
//  Server2
//
//  Created by Jingnong Wang on 2017/2/8.
//  Copyright © 2017年 Jingnong Wang. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT "8888"
#define MAXBUFFER 300

struct packet{
    unsigned short int start_of_packet_id;
    unsigned short int client_id:8;
    unsigned short int packet_type;
    unsigned short int segment_no:8;
    unsigned short int length:8;
    unsigned short int technology:8;
    unsigned int subscriber_no;
    unsigned short int end_of_packet_id;
} packet = {0xFFFF, 0x01, 0x0, 0x1, 0xE, 0x0, 0x0, 0xFFFF};

struct database{
    unsigned int subscriber_no;
    int technology;
    int paid;
} subscribers[3];

char recieved[MAXBUFFER];

/* Setup a UDP socket */
struct addrinfo hints, *servinfo, *p;
struct sockaddr_storage client_addr;
socklen_t addr_len;
char s[INET6_ADDRSTRLEN];
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(void) {
    int socketServer;
    addr_len = sizeof client_addr;
    int rv;
    
    /* Setup a UDP socket */
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((socketServer = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("socket error");
            continue;
        }
        if (bind(socketServer, p->ai_addr, p->ai_addrlen) == -1) {
            close(socketServer);
            perror("bind error");
            continue;
        }
        break;
    }
    if (p == NULL) {
        fprintf(stderr, "failed to bind socket.\n");
        return 2;
    }
    freeaddrinfo(servinfo);
    
    /* Read the database */
    char* filename = "verification_database.txt";
    FILE * pFile;
    char buf[MAXBUFFER];
    char *data[3];
    char *numberGot;
    char numberProcessed[20];
    int i = 0;
    pFile = fopen (filename, "r");
    if (pFile == NULL){
        perror ("Error opening file");
        exit(-1);
    }
    while (fgets(buf,MAXBUFFER,pFile) != NULL){
        numberGot = strtok(buf, "\t");
        subscribers[i].technology = atoi(strtok(NULL, "\t"));
        subscribers[i].paid = atoi(strtok (NULL, "\t"));
        data[0] = strtok(numberGot, "-");
        data[1] = strtok(NULL, " -");
        data[2] = strtok (NULL, " -");
        
        bzero(numberProcessed, sizeof(numberProcessed));
        strcat(numberProcessed, data[0]);
        strcat(numberProcessed, data[1]);
        strcat(numberProcessed, data[2]);
        subscribers[i].subscriber_no = atoi(numberProcessed);
        bzero(buf, MAXBUFFER);
        i++;
    }
    
    
    while (1) {
        printf("Listening to data...\n");
        
        /* Recieve packet from client */
        int recievedlen;
        bzero(recieved, MAXBUFFER);
        memset(&packet, 0, sizeof(packet));
        
        recievedlen = recvfrom(socketServer, recieved, MAXBUFFER - 1 , 0,(struct sockaddr *)&client_addr, &addr_len);
        if (recievedlen == -1) {
            perror("recvfrom error\n");
            exit(1);
        }
        memcpy(&packet, recieved, sizeof(packet));
        printf("The subscriber number is %u \n", packet.subscriber_no);
        printf("The technology is %d \n", packet.technology);

        
        /* Check the database, comparing with the data provided by client */
        int sendlen;
        packet.packet_type = 0xFFFA;
        
        for (int j = 0; j < 3; j++) {
            if (packet.subscriber_no == subscribers[j].subscriber_no && packet.technology == subscribers[j].technology) {
                if (subscribers[j].paid == 1) {
                    packet.packet_type = 0xFFFB;
                    printf("Subscriber got accessed. \n\n"); // subscribers number and tech are all correct. Paid.
                }
                else {
                    packet.packet_type = 0xFFF9;
                    printf("Rejected. Subscriber has not paid \n\n"); // subscribers number and tech are all correct, but not paid.
                }
            }
        }
        if (packet.packet_type == 0xFFFA) {
            printf("Rejected. Subscriber does not exist on database. \n\n"); // subscribers number and tech are not both correct.
        }
        bzero(recieved, MAXBUFFER);
        memcpy(recieved, &packet, sizeof(packet));
        
        /* Send packet */
        sendlen = sendto(socketServer, recieved, MAXBUFFER - 1 , 0,(struct sockaddr *)&client_addr, addr_len);
        if (sendlen == -1) {
            perror("send error");
            exit(1);
        }
    }
    
    fclose (pFile);
    close(socketServer);
    return 0;
}
