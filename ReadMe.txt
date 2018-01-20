The source code of client is in Client2/Client2/main.c
The source code of server is in Server2/Server2/main.c
The database files for server is Server2/verification_database.txt.
The snapshots of client and server running output is Client_Snapshot.jpeg and Server_Snapshot.jpeg.

The client sends 4 packets, representing:
1-correct number and technology (paid)
2-correct number and technology (not paid)
3-correct number and technology (paid)
4-wrong number and technology
respectively.

The server receives these packets and check the number and technology of the packets, send corresponding check message to the client. The client receive the packet from server and display that if it got accessed or not. If the server does not send back massage in 3sec, the client resend the packet. After 3 times resend, client stops sending and confirm that server does not respond.