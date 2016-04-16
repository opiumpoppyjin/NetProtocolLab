#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <assert.h>

#include <pthread.h>
#define NUM_THREADS 5

#define SERV_PORT 4321 
#define CITYLEN 20
#define RECVMSGLEN 23
#define SENDMSGLEN 77
//允许保存的未处理连接请求数量
#define LISTENQ 8

#define bool int 
#define true 1
#define false 0

#pragma(1)//按一个字节对齐
//23byte
typedef struct RecvData{
	char nord;	//city name(0x01) or date(0x02)
	char opday;  //day option:1day(0x01) 3days(0x02)
	char city[CITYLEN];	
	char days;	//date(0x01-0x09)
}RecvData;
//77byte
typedef struct SendData{
	char query; //cityname (correct 0x01,wrong0x02);weather(0x03);noInformation(0x04)
	char opday;	//1day(0x41) 3days(0x42)
	char city[CITYLEN];
	struct { //20 16 03 31
		unsigned char year1;
		unsigned char year2;
		char month;
		char day;
	}date;
	char days; //0x01-0x09
	struct {
		char type;//shower 0x00，clear 0x01，cloudy 0x02，rain 0x03 ，fog 0x04
		char temp;
	}weather[25];
}SendData;
#pragma()

struct{
	char city[20];
}name[]={
"nanjing",
"xining",
"shangha",
"beijing",
"tianjin",
"chongqing",
"chengdu",
"hangzhou",
"xian",
"lanzhou"
};

static int cli_nr=0;

char randtype(){
	return rand()&4;
}

char randtemp(){
	return rand()%40;
}

bool checkname(char *city){
	int i;
	for(i=0;i<10;i++){
		if (strcmp(city,name[i].city)==0)
			return true;
	}
	return false;
}

void *send_and_recv(void *fp){
	int connfd=(int)fp;
	
	RecvData recvdata;
	SendData senddata;
	while(true){
		//receive
		int rc=recv(connfd,&recvdata,RECVMSGLEN,0);
		if (rc<0){
			perror("Receive error: ");
			return;
		}
		else if (rc==0)
			return;
		//fill sendsocket
		if (recvdata.nord==0x01){//ack city
			printf("Client %d request ack city.\n",cli_nr);
			if(checkname(recvdata.city)==true){
				senddata.query=0x01;
			}	
			else
				senddata.query=0x02;	
			strncpy(senddata.city,recvdata.city,CITYLEN);
			send(connfd,&senddata,SENDMSGLEN,0);
			continue;
		}
		else if (recvdata.nord==0x02){//demand weather
			senddata.query=0x03;

			strncpy(senddata.city,recvdata.city,CITYLEN);
			senddata.date.year1=0x07;
			senddata.date.year1=0xe0;
			senddata.date.month=0x04;
			senddata.date.day=0x01;
			senddata.days=recvdata.days;
			if (recvdata.opday==0x01){//1day
				printf("Client %d request the %dth day's weather.\n",cli_nr,recvdata.days);

				if (recvdata.days==0x09){
					senddata.query=0x04;
				}
				senddata.opday=0x41;
				senddata.weather[0].type=randtype();
				senddata.weather[0].temp=randtemp();
			}
			else if (recvdata.opday==0x02){//3days
				printf("Client %d request the 3 days' weather.\n",cli_nr);
				senddata.opday=0x42;
				int i;
				for (i=0;i<3;i++){
					senddata.weather[i].type=randtype();
					senddata.weather[i].temp=randtemp();
				}
			}	
		}

		//send
		if (send(connfd,&senddata,SENDMSGLEN,0)<0){
			perror("Send error: ");
		}
	}
	close(connfd);
}

int main(){
	//set tcp 
	int listenfd=socket(AF_INET,SOCK_STREAM,0);
	//fill tcp addr
	struct sockaddr_in servaddr;
	servaddr.sin_family=AF_INET;
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
	servaddr.sin_port=htons(SERV_PORT);
	//allot tcp addr for tcp
	bind (listenfd,(struct sockaddr *) &servaddr, sizeof(servaddr));
	//listening
	if (listen(listenfd,LISTENQ)<0){
		perror("Can not listen: ");
		return false;
	}
	printf("Server running... waiting for connections.\n");

	struct sockaddr_in cliaddr;
	socklen_t clilen=sizeof(cliaddr);

	pthread_t threads[NUM_THREADS];
	int rc;
	long t;
	for(t=0;t<NUM_THREADS;t++){
		int connfd=accept(listenfd,(struct sockaddr*) &cliaddr, &clilen);
		cli_nr++;
		printf("Creating thread %ld\n",t);
		rc=pthread_create(&threads[t],NULL,send_and_recv,(void *)connfd);
		if (rc){
			printf("ERROR!");
			exit(-1);
		}
	}
	return true;
}


