//
//  main.c
//  Client2
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
#include <signal.h>
#include <sys/time.h>

#define PORT "8888"
#define SERVER "127.0.0.1"
#define BUF_SIZE 300

struct packet {
    unsigned short int start_of_packet_id;
    unsigned short int client_id:8;
    unsigned short int accper;
    unsigned short int segment_no:8;
    unsigned short int length:8;
    unsigned short int technology:8;
    unsigned int subscriber_no;
    unsigned short int end_of_packet_id;
} packet = {0xFFFF, 0x01, 0x0, 0x1, 0xE, 0x0, 0x0, 0xFFFF};

char sent[BUF_SIZE];
char recieved[BUF_SIZE];

struct addrinfo hints, *servinfo, *p;

void sendPacket(int, int, int, unsigned int);

int main() {
    int sockClient;
    int packet_num = 1;
    int send_counter;
    char sub_no[20];
    char tech[20];
    unsigned int subscriber_no;
    int technology;
    int rv;
    
    /* Creat a UDP socket client */
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    if ((rv = getaddrinfo(SERVER, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return -1;
    }
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockClient = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("socket error");
            continue;
        }
        break;
    }
    if (p == NULL) {
        fprintf(stderr, "failed to create socket\n");
        return -1;
    }
    
    /* Set ack timer */
    struct timeval timeout;
    timeout.tv_sec  = 3;
    timeout.tv_usec = 0;
    setsockopt(sockClient, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
    
    /* Send packets */
    while (1) {
        send_counter = 0;
        rv = -1;

        printf("\nEnter subscriber number: \n");
        scanf("%s",sub_no);
        subscriber_no = atoi(sub_no);
        
        printf("Enter technology type: \n");
        scanf("%s",tech);
        technology = atoi(tech);
        
        sendPacket(sockClient, packet_num, technology, subscriber_no);
        memset(recieved,0,BUF_SIZE);
        
        /* Listen for ack, count time and resend packet if time out */
        while (send_counter < 4 && rv < 0){
            rv = recvfrom(sockClient, recieved, BUF_SIZE-1, MSG_WAITALL, p->ai_addr, &p->ai_addrlen);
            if(rv == -1 && errno == EAGAIN){
                if(send_counter == 3){
                    printf("Server does not Respond.\n");
                    return -1;
                }
                send_counter++;
                printf("Did not receive ack in time. Resend the %d time.\n", send_counter);
                sendPacket(sockClient, packet_num, technology, subscriber_no);
            }
        }
        packet_num ++;
        
        /* Check received package */
        memcpy(&packet, recieved, sizeof(packet));
        printf("Recieved packet type: \"%#X\"\n", packet.accper);
        
        switch(packet.accper) {
            case 0xFFF9:
                printf("Subscriber is reject. Error message: Subscriber has not paid. %#X \n\n", packet.accper);
                break;
            case 0xFFFA:
                printf("Subscriber is reject. Error message: Subscriber does not exist on database. %#X \n\n", packet.accper);
                break;
            case 0xFFFB:
                printf("Subscriber got accessed.\n\n", packet.technology);
                break;
        }
    }
    
    freeaddrinfo(servinfo);
    close(sockClient);
    return 0;
}

void sendPacket(int sockClient, int number, int technology, unsigned int subscriber_no) {
    int sendlen;
    printf("Sending packet %d...\n", number);
    
    /* Setup sending packet */
    packet.accper = 0XFFF8;
    packet.segment_no = number;
    packet.technology = technology;
    packet.subscriber_no = subscriber_no;
    memset(sent, 0, BUF_SIZE);
    memcpy(sent, &packet, sizeof(packet));
    
    /* send packet */
    sendlen = sendto(sockClient, sent, sizeof(packet), 0, p->ai_addr, p->ai_addrlen);
    if (sendlen == -1) {
        perror("send error");
        exit(1);
    }
    printf("Packet sent. Sent %d bytes\n", sendlen);
}
