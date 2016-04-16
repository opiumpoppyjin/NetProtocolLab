#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

#include <assert.h>
//server moniter port
#define SERV_PORT 4321 
#define CITYLEN 20
#define SENDMSGLEN 23
#define RECVMSGLEN 77

#define bool int 
#define true 1
#define false 0

int sockfd=0;
int system(const char *string);

#pragma(1)//按一个字节对齐
//23byte
typedef struct SendData{
	char nord;	//city name(0x01) or date(0x02)
	char opday;  //day option:1day(0x01) 3days(0x02)
	char city[CITYLEN];	
	char days;	//date(0x01-0x09)
}SendData;
//77byte
typedef struct RecvData{
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
}RecvData;
#pragma()

bool establish(char *serv_IPaddr){
	struct sockaddr_in servaddr;
	//establish a socket
	sockfd = socket(AF_INET, SOCK_STREAM,0);
	//fill server socket addr
	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_addr.s_addr=inet_addr(serv_IPaddr);
	servaddr.sin_port=htons(SERV_PORT);

	if (connect(sockfd,(struct sockaddr_in *) & servaddr,sizeof(servaddr))<0){
		return false;
	}
	return true;
}

void ui1(){
	printf("===================================================\n");
	printf("Welecome to ZWJS Weather Forecast Application!\n");
	printf("Please input City name in chinese pinyin.\n");
	printf("<# for exit>		<c for cls>\n");
	printf("===================================================\n");
}

void ui2(){
	printf("===================================================\n");
	printf("Please enter the given number to query:\n");
	printf("1.today\n");
	printf("2.three days from today\n");
	printf("3.custom day by yourself\n");
	printf("<r>back,<c>cls,<#>exit\n");
	printf("===================================================\n");
}

bool send_ack_cityname(char* city){
	SendData data={0x01,0x00};
	strncpy(data.city,city,20);
	data.days=0x00;
	if (send(sockfd,&data,SENDMSGLEN,0)<0){
		perror("Senderror:");
		return false;
	}
	return true;
}

bool send_day(int op,char *city,int ch){
	SendData data;
	data.nord=0x02;
	if (op==1) {
		data.opday=0x01;
		data.days=0x01;
	}
	else if (op==2) {
		data.opday=0x02;
		data.days=0x03;
	}
	else if (op==3){
		data.opday=0x01;
		data.days=ch;
	}
	strncpy(data.city,city,20);
	if (send(sockfd,&data,SENDMSGLEN,0)<0){
		perror("Senderror:");
		return false;
	}
	return true;
}

void parseWeatherType(char type){
	char *t;
	switch(type){
		case 0x00:t="shower";break;
		case 0x01:t="clear";break;
		case 0x02:t="cloudy";break;
		case 0x03:t="rain";break;
		case 0x04:t="fog";break;
		default: assert(0);
	}
	printf("%s; ",t);
}
bool parseRecvSocket(){
	RecvData data;
	recv(sockfd,&data,RECVMSGLEN,0);
	//parse query.
	switch (data.query){
		case 0x01: return true;
		case 0x02: 
			printf("Input error: This city is not exist!\n");
			return false;
		case 0x03: break;
		case 0x04: 
			printf("Sorry no given day's information for city %s!\n",data.city);
			return true;
		default: assert(0);
	}
	//parse others.
	printf ("City: %s	Today is %d/%d/%d Weather information is as follows:\n",data.city,((unsigned int)data.date.year1<<8)+(unsigned int)data.date.year2,data.date.month,data.date.day);
	if (data.opday==0x41) {
		if (data.days==0x01){
			printf("Today's weather is: ");
		}
		else{
			printf("The %dth day's weather is: ",data.days);
		}
		parseWeatherType(data.weather[0].type);
		printf("Temp: %d\n",data.weather[0].temp);
	}
	else if (data.opday==0x42){
		int i,n=3;
		for (i=0;i<n;i++){
			printf("The %dth day's weather is: ",i);
			parseWeatherType(data.weather[i].type);
			printf("Temp: %d\n",data.weather[i].temp);
		}
	}
	return true;
}

int main(int argc, char **argv){
	if (argc!=2){
		perror("Just input IP addr! error: ");
	}
	assert(establish(argv[1])==true);

	ui1();	
	char *input=(char*)malloc(CITYLEN);	
	while(1){
		if (scanf("%s",input)<=0){	perror("Error!"); }
		if(strcmp("#",input)==0){ return 0; }
		else if(strcmp("c",input)==0){ system("cls"); ui1();}
		else{
			char city[CITYLEN];
			strncpy(city,input,20);
			if (send_ack_cityname(city)==false) continue;
			if (parseRecvSocket()==false) continue;
			ui2();
			while(1){
				if (scanf("%s",input)<=0){ perror("Error!"); }
				else if (strcmp("r",input)==0){ break; }
				else if (strcmp("c",input)==0){ 
					system("clear"); 
					ui2();
				}
				else if (strcmp("#",input)==0){ return 0;}
				else if (strcmp("1",input)==0){
					send_day(1,city,0);
					parseRecvSocket();
				}
				else if (strcmp("2",input)==0){
					send_day(2,city,0);
					parseRecvSocket();
				}
				else if (strcmp("3",input)==0){
					printf("Please enter the day number(1-9): ");
					if (scanf("%s",input)<=0){ perror("Error!"); }
					int ch=atoi(input);
					if(ch>=1&&ch<=9){
						send_day(3,city,ch);
						parseRecvSocket();
					}
					else{
						printf("Input error!\n");	
					} 
				}
				else{
					printf("Input error!\n");	
				} 
			}
			ui1();
		}
	}
	free(input);
	return 0;
}
