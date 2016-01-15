#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <netdb.h>

//sockfd: main server
//sockp2p: local p2p server
//newsockp2p: p2p clients

int sockfd;

void error(char *msg){
	perror(msg);
	exit(1);
};

void* serve_peer(void* passsockp2p){
	int pssocketp2p = *(int*)passsockp2p;
	free(passsockp2p);
	int n;
	char buffertt[1024], temptt[1024];
	bzero(buffertt, 1024);
	n = read(pssocketp2p, buffertt, 1024);//get <fromname>#<payment>
	if(n<0)
		error("ERROR on reading");
	else{
		//send msg to main server for payment
		//concat buffer with protocol $$$<fromname>#<payment>
		bzero(temptt, 1024);
		strcpy(temptt, "$$$");
		strcat(temptt, buffertt);
		//send buffertt to main server
		n = write(sockfd, temptt, 1024);
		if(n<0)
			error("ERROR on writing");
		//read reply from main server
		bzero(buffertt, 1024);
		n = read(sockfd, buffertt, 1024);
		if(n<0)
			error("ERROR on reading");
		printf("%s", buffertt);
		fflush(stdout);
		//send reply to peer
		n = write(pssocketp2p, buffertt, 1024);
		if(n<0)
			error("ERROR on writing");
		//close thread
		pthread_exit(NULL);
	}
};

void* waiting_p2p(void* pt){
	int sockp2p, newsockp2p, clilen, port = *(int*)pt, n;
	free(pt);
	struct sockaddr_in serv_addr, cli_addr;
	char buffert[1024], tempt[1024];
	sockp2p = socket(AF_INET, SOCK_STREAM, 0);
	if(sockp2p<0)
		error("ERROR opening socket");
	bzero((char*) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	if(bind(sockp2p, (struct sockaddr*) &serv_addr, sizeof(serv_addr))<0)
		error("ERROR on binding");
	listen(sockp2p, 5);
	clilen = sizeof(cli_addr);
	while(newsockp2p=accept(sockp2p, (struct sockaddr*) &cli_addr, &clilen)){
		if(newsockp2p<0)
			error("ERROR on accept");
		else{
			bzero(buffert, 1024);
			int* passsockp2p = malloc(sizeof(int));
			*passsockp2p = newsockp2p;
			pthread_t tidp2p;
			pthread_create(&tidp2p, NULL, serve_peer, passsockp2p);
		}
	}
};

void p2p(char *login_port){
	int *pt = malloc(sizeof(int));
	*pt = atoi(login_port);
	pthread_t tid;
	pthread_create(&tid, NULL, waiting_p2p, pt);
};

int main(int argc, const char *argv[])
{
	int port, n;
	struct sockaddr_in serv_addr;
	struct hostent *server;
	char buffer[1024], temp[25], createport[25];
	bool command_s, exit_s, serv_s = false, logined;
	logined = false;
	if(argc<3){
		fprintf(stderr, "usage %s hostname port", argv[0]);
		exit(0);
	}
	port = atoi(argv[2]);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd<0)
		error("ERROR opening socket");
	server = gethostbyname(argv[1]);
	if(server==NULL){
		fprintf(stderr, "ERROR, no such host");
		exit(0);
	}
	bzero((char*) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	bcopy((char*) server->h_addr, (char*) &serv_addr.sin_addr.s_addr, server->h_length);
	serv_addr.sin_port = htons(port);
	if(connect(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr))<0)
		error("ERROR connecting");
	printf("Welcome to the system\n");
	bzero(buffer, 1024);
	n = read(sockfd, buffer, 1024);
	printf("%s", buffer);
	while(1){
		printf("\n----------------------------------------------------\n");
		if(!logined)
			printf("1. Register  2. Log In\n");
		else
			printf("1. Account List  2. Exit  3. Pay\n");
		printf("Please enter the service number: ");
		bzero(buffer, 1024);
		scanf("%s", buffer);
		switch(atoi(buffer)){
			case 1:
				if(!logined){
					do{
						printf("Please enter an account name (within 9 characters): ");
						bzero(temp, 25);
						scanf("%s", temp);
						if(strlen(temp)>9)
							printf("Account name is too long, please enter again.\n");
					}while(strlen(temp)>9);
					bzero(buffer, 1024);
					strcpy(buffer, "REGISTER#");
					strcat(buffer, temp);
					bzero(temp, 25);
					printf("Please enter your account balance: ");
					scanf("%s", temp);
					strcat(buffer, "$");
					strcat(buffer, temp);
					command_s = true;
					exit_s = false;
				}
				else{
					bzero(buffer, 1024);
					strcat(buffer, "List");
					command_s = true;
					exit_s = false;
				}
					break;
			case 2:
				if(!logined){
					printf("Please enter your account name: ");
					bzero(buffer, 1024);
					scanf("%s", buffer);
					printf("Please enter your port number: ");
					bzero(temp, 25);
					scanf("%s", temp);
					bzero(createport, 25);
					strcpy(createport, temp);
					strcat(buffer, "#");
					strcat(buffer, temp);
					command_s = true;
					exit_s = false;
				}
				else{
					bzero(buffer, 1024);
					strcat(buffer, "Exit");
					command_s = true;
					exit_s = true;
				}
				break;
			case 3:
				if(!logined){
					printf("Unknown command, please retry\n");
					command_s = false;
					exit_s = false;
				}
				else{//Send "Search#<username>" and get "<ip>#<port>"
					printf("Please enter the account name for payment: ");
					bzero(buffer, 1024);
					strcpy(buffer, "Search#");
					bzero(temp, 25);
					scanf("%s", temp);
					strcat(buffer, temp);
					n = write(sockfd, buffer, 1024);
					if(n<0)
						error("ERROR writing to socket");
					bzero(buffer, 1024);
					n = read(sockfd, buffer, 1024);
					if(buffer[0]=='F'){
						printf("%s", buffer);
						fflush(stdout);
						command_s = false;
						exit_s = false;
					}
					else{//below tell another client for payment, msg to peer: $$$<myname>#<balance>
						char rcvip[25];
						int rcvport, numbersit;
						bzero(rcvip, 25);
						for(n=0;n<strlen(buffer);n++){
							if(buffer[n]=='#'){
								numbersit = n;
								break;
							}
							rcvip[n] = buffer[n];
						}
						bzero(temp, 25);
						for(n=numbersit+1;n<strlen(buffer);n++){
							temp[n-numbersit-1] = buffer[n];
						}
						rcvport = atoi(temp);
						printf("How much for payment: ");
						bzero(temp, 25);
						scanf("%s", temp);
						//create a socket to connect client
						//ip: rcvip, port: rcvport
						printf("%s, %d\n", rcvip, rcvport);
						struct sockaddr_in serv_addr_t;
						struct hostent *server_t;
						int sockfd_t;
						sockfd_t = socket(AF_INET, SOCK_STREAM, 0);
						if(sockfd_t<0)
							error("ERROR opening socket");
						server_t = gethostbyname(rcvip);
						if(server_t==NULL){
							fprintf(stderr, "ERROR, no such host");
							exit(0);
						}
						bzero((char*) &serv_addr_t, sizeof(serv_addr_t));
						serv_addr_t.sin_family = AF_INET;
						bcopy((char*) server_t->h_addr, (char*) &serv_addr_t.sin_addr.s_addr, server->h_length);
						serv_addr_t.sin_port = htons(rcvport);
						if(connect(sockfd_t, (struct sockaddr*) &serv_addr_t, sizeof(serv_addr_t))<0)
							error("ERROR connecting");
						//search myname
						bzero(buffer, 1024);
						strcpy(buffer, "SearchMyname");
						n = write(sockfd, buffer, 1024);
						if(n<0)
							error("ERROR on writing");
						bzero(buffer, 1024);
						n = read(sockfd, buffer, 1024);
						if(n<0)
							error("ERROR on reading");
						//send <myname>#<payment> to peer
						strcat(buffer, "#");
						strcat(buffer, temp);
						n = write(sockfd_t, buffer, 1024);
						if(n<0)
							error("ERROR on writing");
						bzero(buffer, 1024);
						n = read(sockfd_t, buffer, 1024);
						if(n<0)
							error("ERROR on reading");
						printf("%s", buffer);
						fflush(stdout);

						command_s = false;
						exit_s = false;
					}
				}
				break;
			default:
				printf("Unknown command, please retry.\n");
				command_s = false;
				exit_s = false;
				break;
		}
		if(command_s){
			n = write(sockfd, buffer, strlen(buffer));
			if(n<0)
				error("ERROR writing to socket");
			bzero(buffer, 1024);
			n = read(sockfd, buffer, 1024);
			if(buffer[0] == 'A'){
				logined = true;
				if(!serv_s){
					serv_s = true;
					p2p(createport);
				}
			}
			if(n<0)
				error("ERROR reading from socket");
			printf("%s", buffer);
		}
		if(exit_s){
			printf("\n");
			close(sockfd);
			break;
		}
	}

	return 0;
}
