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

int sockfd, port, clilen, n, online_number = 0;
struct sockaddr_in serv_addr, cli_addr;
char buffer[1024], temp[1024];
struct list *head_list= NULL;

void error(char *msg){
	perror(msg);
	exit(1);
};

struct user{
	char name[20];
	int balance;
};

struct list{
	struct user u;
	char ip_addr[15];
	int port;
	struct list* next;
};

void account_list(struct list* myUser, char* buffert){
	bzero(buffert, 1024);
	char temp[100];
	sprintf(buffert, "Account: %s, Balance: %d\n%d online users.\n", myUser->u.name, myUser->u.balance, online_number);
	struct list* cur;
	int count = 1;
	for(cur=head_list;cur!=NULL;cur = cur->next){
		bzero(temp, 100);
		sprintf(temp, "The %d's username is %s. From IP: %s and port: %d\n", count, cur->u.name, cur->ip_addr, cur->port);
		strcat(buffert, temp);
		count++;
	}
	
};

void* thread_go(void* psocket){
	int passsocket = *(int*) psocket;
	char buffert[1024], tempt[1024];
	struct list* me = malloc(sizeof(struct list));
	bzero(me->u.name, 20);
	printf("Connecting to a client!\n");
	fflush(stdout);
	bzero(buffert, 1024);
	while(n=read(passsocket, buffert, 1024)){
		if(n<0)
			error("ERROR reading from socket");
		bool loginned = false, isReg=false;
		int numbersit;
		if(strncmp(buffert, "REGISTER#", 9)==0){
			bzero(tempt, 1024);
			for(n=9;n<strlen(buffert);n++){
				if(buffert[n]=='$'){
					numbersit = n;
					break;
				}
				tempt[n-9] = buffert[n];
			}
			char tempt2[25];
			bzero(tempt2, 25);
			for(n=numbersit+1;n<strlen(buffert);n++)
				tempt2[n-numbersit-1] = buffert[n];
			for(n=0;n<25;n++)
				if(tempt2[n]=='\0')
					tempt2[n] = ' ';
			FILE *fp;
			fp = fopen("register.txt", "r");
			if(fp==NULL)
				error("ERROR on open file");
			bzero(buffert, 1024);
			isReg = false;
			if(strcmp("REGISTER", tempt)!=0&&strcmp("List", tempt)!=0&&strcmp("Exit", temp)!=0){
				while(fgets(buffert, 1024, fp)!=NULL){
					if(strncmp(tempt, buffert, strlen(tempt))==0){
						isReg = true;
						break;
					}
					fgets(buffert, 1024, fp);
					bzero(buffert, 1024);
				}
			}
			fclose(fp);
			bzero(buffert, 1024);
			if(!isReg){

				strcat(tempt, "\n");
				strcat(tempt, tempt2);
				strcat(tempt, "\n");
				fp = fopen("register.txt", "a");
				fwrite(tempt, 1, strlen(tempt), fp);
				fclose(fp);
				strcpy(buffert, "100 OK\n");
			}
			else
				strcpy(buffert, "210 FAIL\n");
		}
		else if(strncmp(buffert, "List", 4)==0 && strlen(buffert)==4){
			account_list(me, buffert);
			printf("%s: List\n", me->u.name);
			fflush(stdout);
		}
		else if(strncmp(buffert, "Exit", 4)==0 && strlen(buffert)==4){
			printf("%s leaved!\n", me->u.name);
			fflush(stdout);
			struct list *cur = head_list, *pre = NULL;
			while(true){
				if(cur==me){
					if(pre==NULL)
						head_list = cur->next;
					else{
						pre->next = cur->next;
					}
					free(cur);
					cur = NULL;
					pre = NULL;
					me = NULL;
					online_number--;
					break;
				}
				else{
					pre = cur;
					cur = cur->next;
				}
			}
			bzero(buffert, 1024);
			strcpy(buffert, "bye!\n");
			n = write(passsocket, buffert, 1024);
			free(psocket);
			pthread_exit(NULL);
		}
		else if(strncmp(buffert, "Search#", 7)==0){//reply for request ip and port
			bzero(tempt, 1024);
			int i;
			for(i=7;i<strlen(buffert);i++){
				tempt[i-7] = buffert[i];
			}
			struct list *cur = head_list;
			while(cur!=NULL){
				if(strncmp(cur->u.name, tempt, strlen(tempt))==0)
					break;
				cur = cur->next;
			}
			if(cur==NULL){
				bzero(buffert, 1024);
				strcpy(buffert, "Failed, no such online user!\n");
			}
			else{
				bzero(buffert, 1024);
				strcpy(buffert, cur->ip_addr);
				strcat(buffert, "#");
				bzero(tempt, 1024);
				sprintf(tempt, "%d", cur->port);
				strcat(buffert, tempt);
			}
		}
		else if(strncmp(buffert, "SearchMyname", 12)==0){//send myname
			bzero(buffert, 1024);
			strcpy(buffert, me->u.name);
		}
		else if(strncmp(buffert, "$$$", 3)==0){//payment success or not, get $$$<fromname>#<payment>
			//check if the account balance is more than payment
			char fromname[25];
			int payment, numbersit;
			bzero(fromname, 25);
			for(n=3;n<strlen(buffert);n++){
				if(buffert[n]=='#'){
					numbersit = n;
					break;
				}
				fromname[n-3] = buffert[n];
			}
			bzero(tempt, 1024);
			for(n=numbersit+1;n<strlen(buffert);n++)
				tempt[n-numbersit-1] = buffert[n];
			payment = atoi(tempt);
			bool success = false;
			struct list *from, *to, *cur = head_list;
			while(cur!=NULL){
				if(strncmp(cur->u.name, fromname, strlen(fromname))==0)
					from = cur;
				if(strncmp(cur->u.name, me->u.name, strlen(me->u.name))==0)
					to = cur;
				cur = cur->next;
			}
			if(from->u.balance>=payment)
				success = true;
			if(success){//if success
			//change the online temporary balance
				from->u.balance -= payment;
				to->u.balance += payment;
			//change the file balance
				FILE *fp;
				int pos = 0;
				fp = fopen("register.txt", "r+");
				while(fgets(buffert, 1024, fp)){
					pos += strlen(buffert);
					char bal[25];
					if(strncmp(buffert, from->u.name, strlen(from->u.name))==0){
						bzero(bal, 25);
						sprintf(bal, "%d", from->u.balance);
						for(n=0;n<25;n++)
							if(bal[n]=='\0')
								bal[n] = ' ';
						int fpt = fseek(fp, pos, SEEK_SET);
						if(fpt<0)
							error("fseek");
						fprintf(fp, "%s", bal);
						pos += 25;
					}
					else if(strncmp(buffert, to->u.name, strlen(to->u.name))==0){
						bzero(bal, 25);
						sprintf(bal, "%d", to->u.balance);
						for(n=0;n<25;n++)
							if(bal[n]=='\0')
								bal[n] = ' ';
						int fpt = fseek(fp, pos, SEEK_SET);
						if(fpt<0)
							error("fseek");
						fprintf(fp, "%s", bal);
						pos += 25;
					}
				}
			//send a msg for success
				bzero(buffert, 1024);
				strcpy(buffert, "SUCCESS PAYMENT\n");
			}
			else{//else fail
				bzero(buffert, 1024);
				strcpy(buffert, "FAIL PAYMENT\n");
			}
		}
		else{//login
			int no_pos, i;
			bool is_double_loginned=false;
			bzero(tempt, 1024);
			for(no_pos=0;no_pos<strlen(buffert);no_pos++){
				if(buffert[no_pos]=='#')
					break;
				tempt[no_pos] = buffert[no_pos];
			}
			struct list *cur = head_list;
			while(cur!=NULL){
				if(strncmp(tempt, cur->u.name, strlen(tempt))==0)
					is_double_loginned = true;
				cur = cur->next;
			}
			if(!is_double_loginned){
				strcpy(me->u.name, tempt);
				bzero(tempt, 1024);
				for(i=no_pos+1;i<strlen(buffert);i++){
					tempt[i-no_pos-1] = buffert[i];
				}
				me->port = atoi(tempt);
				strcpy(me->ip_addr, inet_ntoa(cli_addr.sin_addr));//get client ip
				FILE *fp;
				fp = fopen("register.txt", "r");
				bzero(tempt, 1024);
				bzero(buffert, 1024);
				strcpy(tempt, me->u.name);
				while(fgets(buffert, 1024, fp)!=NULL){
					if(strncmp(tempt, buffert, strlen(tempt))==0){
						loginned = true;
						break;
					}
					fgets(buffert, 1024, fp);
					bzero(buffert, 1024);
				}
				if(loginned){
					bzero(buffert, 1024);
					fgets(buffert, 1024, fp);
					if(buffert[strlen(buffert)-1]=='\n')
						buffert[strlen(buffert)-1]='\0';
					me->u.balance = atoi(buffert);
					me->next = head_list;
					head_list = me;
					online_number++;
					fclose(fp);
					account_list(me, buffert);
					printf("%s logged in!\n", me->u.name);
					fflush(stdout);
				}
				else{
					bzero(buffert, 1024);
					strcpy(buffert, "Failed, no such account.\n");
				}
			}
			else{
				bzero(buffert, 1024);
				strcpy(buffert, "Failed, double loggined.\n");
			}
		}
		n = write(passsocket, buffert, 1024);
		if(n<0)
			error("ERROR writing socket.");
		bzero(buffert, 1024);
	}
	if(me->u.name[0]!='\0'){
		struct list *cur = head_list, *prev = NULL;
		while(cur!=NULL){
			if(cur==me)
				break;
			prev = cur;
			cur = cur->next;
		}
		if(prev!=NULL)
			prev->next = cur->next;
		else
			head_list = cur->next;
		free(cur);
		online_number--;
	}
	printf("A client disconnected\n");
	fflush(stdout);
	pthread_exit(NULL);

};

int main(int argc, const char *argv[]){
	pthread_t tid = 0;
	int newsockfd;
	if(argc<2){
		fprintf(stderr, "ERROR, no port provided");
		exit(1);
	}
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd<0)
		error("ERROR opening socket");
	bzero((char*) &serv_addr, sizeof(serv_addr));
	port = atoi(argv[1]);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	if(bind(sockfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr))<0)
		error("ERROR on binding");
	listen(sockfd, 5);
	clilen = sizeof(cli_addr);
	while(newsockfd=accept(sockfd, (struct sockaddr*) &cli_addr, &clilen)){
		if(newsockfd<0)
			error("ERROR on accept");
		else{
			bzero(buffer, 1024);
			n = write(newsockfd, "Welcome! Please Register or Login!\n", 100);
			if(n<0)
				error("ERROR writing to socket");
			int* passsocket = malloc(sizeof(int));
			*passsocket = newsockfd;
			pthread_create(&tid, NULL, thread_go, passsocket);
		}
	}

	return 0;

}
