#include "server.h"
/********** init **********/
void init_list(){
	int i;
	for (i=0;i<MAXPLAYER;i++){
		list[i].state=0;
		list[i].score=0;
	}
}
void init_games(){
	int i;
	for (i=0;i<MAXPLAYER;i++){
		games[i].use=0;
	}
}

void init(){
	init_list();
	init_games();
	signal_list=0;
	signal_askChal=0;
	signal_replyChal=0;
}
/********** listen **********/
void *listenUserlist(){
	while(true){
		if (signal_list==1){
			/*send changinfo for every client*/
			int i;
			for(i=0;i<MAXPLAYER;i++){
				if (list[i].state!=0){
					if (SD.type!=9&&SD.type!=10)
						strncpy(SD.username,list[i].name,NAMELEN);
					if (send(list[i].connfd,&SD,SENDMSGLEN,0)<0){
						perror("Send error(listenUserlist) ");
					}
				}
			}
			signal_list=0;
			/*TODO unlock SD*/
		}	
	}
}
void *listenChalask(void *fp){
	int connfd=(int)fp;
	while(true){
		if (signal_askChal==1){
			if (chalConnfd.user2==connfd){
				signal_askChal=0;
				signal_replyChal=1;
			}
			else{
				/*TODO stop until reply==0,continue*/
				while(signal_askChal==1);
				continue;
			}
			/*TODO send chalask*/
			SendData sd;
			sd.type=7;
			strcpy(sd.username,chalname.user2);
			strcpy(sd.challenger,chalname.user1);

			if (send(connfd,&sd,SENDMSGLEN,0)<0){
				perror("Send error(listenChalask) ");
			}
		}	
		/*TODO wait until signal_list chang to 0*/
	}
}
/********** login and logout **********/
int userLogin(RecvData recvdata,int connfd,int pid){
	SendData sd;
	int i;
	/*check username if is exist*/
	for (i=0;i<MAXPLAYER;i++){

		/*login failed: username is exist.*/
		if (list[i].state!=0&&strcmp(list[i].name,recvdata.username)==0){
			sd.type=1;
			sd.state=0;
			strncpy(sd.username,recvdata.username,NAMELEN);
			sd.errorNO=0;
			if (send(connfd,&sd,SENDMSGLEN,0)<0){
				perror("Send error(userlogin1) ");
			}
			return -1;
		}
	}
	/*TODO lock list*/
	/*Get a position in userlist*/
	for (i=0;i<MAXPLAYER;i++){
		if (list[i].state==0) break;
	}
	/*login failed: userlist is full.*/
	if (i==MAXPLAYER){
		/*TODO unlock list*/
		sd.type=1;
		sd.state=0;
		strncpy(sd.username,recvdata.username,NAMELEN);
		sd.errorNO=5;
		if (send(connfd,&sd,SENDMSGLEN,0)<0){
			perror("Send error(userlogin2) ");
		}
		return -1;
	}
	/*Login successful*/
	list[i].state=1;
	list[i].connfd=connfd;
	list[i].pid=pid;
	strncpy(list[i].name,recvdata.username,NAMELEN);
	list[i].score=0;
	/*TODO unlock list*/
	/*fill login query 1*/
	sd.type=1;
	sd.state=1;
	strncpy(sd.username,recvdata.username,NAMELEN);
	sd.user_score=0;
	sd.challenger_score=0;

	printf("Login : Thread ID: %lu  name: %s\n",list[i].pid,list[i].name);

	if (send(connfd,&sd,SENDMSGLEN,0)<0){
		perror("Send error(userlogin2.5) ");
	}

	/*send userlist*/
	/*TODO lock list*/
	SendUserlist su;
	memset(&su, 0, sizeof(SendUserlist));
	su.num=0;
	int j;
	for(j=0;j<MAXPLAYER;j++){
		if (list[j].state!=0){
			strncpy(su.userlist[j].username,list[j].name,NAMELEN);
			/*TODO blood*/
			if (list[j].state==1)
				su.userlist[j].state=1;
			else if (list[j].state==2)
				su.userlist[j].state=0;
			su.userlist[j].score=htonl(list[j].score);
			su.num++;
		}
		else {
			su.userlist[j].state=3;
		}
	}
	/*TODO unlock list*/

	su.num = htonl(su.num);

	if (send(connfd,&su,SENDLISTLEN,0)<0){
		perror("Send error(userlogin3) ");
	}
	while(signal_list);
	/*TODO lock SD*/
	SD.type=5;
	SD.state=1;
	SD.user_score=0;
	strncpy(SD.challenger,recvdata.username,NAMELEN);
	signal_list=1;
	return i;
}
/*
   void endUexp(char *name){
   int i;
   for (i=0;i<MAXPLAYER;i++){
   if (strcmp(recvdata.username,list[i].name)==0){
   list[i].state=1;
   list[i].win_nr++;
   }	
   }
   / *TODO send error message to this user* /
   }
   */
void userExit(RecvData recvdata){
	int i;
	for (i=0;i<MAXPLAYER;i++){
		if (strcmp(recvdata.username,list[i].name)==0){
			/*TODO exit when playing*/
			//	if (list[i].state==2){
			//		endUnexp(list[i].GameInfo.enemyname);
			//	}
			list[i].state=0;
			break;
		}
	}
	if (i==MAXPLAYER){
		assert(0);
	}
	/*TODO lock list*/
	list[i].state=0;
	while(signal_list);
	/*TODO unlock list*/
	printf("Logout : Thread ID: %ld  name: %s\n",list[i].pid,list[i].name);
	/* fill send SD.*/
	/*TODO lock SD*/
	SD.type=6;
	strncpy(SD.challenger,recvdata.username,NAMELEN);
	signal_list=1;
}
/********** ask reply playing **********/
short initplaying(short u1,short u2){
	strncpy(list[u1].enemyname,list[u2].name,NAMELEN);
	strncpy(list[u2].enemyname,list[u1].name,NAMELEN);
	int i;
	for(i=0;i<MAXPLAYER;i++){
		if (games[i].use==0)
			break;		
	}
	list[u1].gamesNO=i;
	list[u2].gamesNO=i;
	list[u1].state=2;
	list[u2].state=2;
	games[i].use=1;
	games[i].listNO.user1=u1;
	games[i].listNO.user2=u2;
	games[i].blood.user1=5;
	games[i].blood.user2=5;
	games[i].nr=0;
	games[i].u2ok=0;
	while (signal_list);
	SD.type=9;
	strncpy(SD.username,list[u1].name,NAMELEN);
	strncpy(SD.challenger,list[u2].name,NAMELEN);

	signal_list=1;
	return i;
}

void askChal(int u1,RecvData recvdata){
	int i;
	for (i=0;i<MAXPLAYER;i++){
		if (strcmp(recvdata.user2name,list[i].name)==0){
			break;
		}
	}
	if (list[i].state==2){
		/*TODO send query 0*/
		SendData sd;
		sd.type=8;
		sd.state=0;
		strcpy(sd.username,recvdata.username);
		strcpy(sd.challenger,recvdata.user2name);

		if (send(list[u1].connfd,&sd,SENDMSGLEN,0)<0){
			perror("Send error(askChal) ");
		}	

		return;
	}
	if (list[i].state==1){
		short u2=i;
		chalConnfd.user1=list[u1].connfd;
		chalConnfd.user2=list[u2].connfd;
		strcpy(chalname.user1,recvdata.username);
		strcpy(chalname.user2,recvdata.user2name);
		signal_askChal=1;
		while (signal_askChal);
		while (signal_replyChal);
		SendData sd;
		sd.type=8;
		strcpy(sd.username,recvdata.username);
		strcpy(sd.challenger,recvdata.user2name);
		if (replyOK==0){
			sd.state=0;
		}
		else if (replyOK==1){
			sd.state=1;
		}

		if (send(list[u1].connfd,&sd,SENDMSGLEN,0)<0){
			perror("Send error(askChal) ");
		}	

		printf("Ask challenge : Thread ID: %ld  name: %s\n",list[u1].pid,list[u1].name);

		if (replyOK==1)
			initplaying(u1,u2);
	}
	else{
		assert(0);
	}
}

void replyChal(int u2,RecvData recvdata){
	if (strcmp(recvdata.username,chalname.user2)!=0)
		return;
	if (recvdata.type==5){
		replyOK=1;
	}
	else if (recvdata.type==6)
		replyOK=0;
	else 
		assert(0);
	signal_replyChal=0;
	printf("Reply challenge: Thread ID: %ld  name: %s\n",list[u2].pid,list[u2].name);
}

void playing(short listNO,RecvData recvdata){
	short gameNO=list[listNO].gamesNO;
	/*1.fill games 2.fill sd use games*/
	if (listNO==games[gameNO].listNO.user1){
		games[gameNO].WordsInfo[games[gameNO].nr].user1=recvdata.attack;
	//	int timer=0;
		while (games[gameNO].u2ok==0){
			/*timeout attack 0!!*/
			/*if (timer>9000000){
				games[gameNO].WordsInfo[games[gameNO].nr].user2=0;
				games[gameNO].u2ok=1;
			}
			timer++;*/
		}
		/*who is winner in this attack*/
		if (games[gameNO].WordsInfo[games[gameNO].nr].user1==games[gameNO].WordsInfo[games[gameNO].nr].user2){
			games[gameNO].WordsInfo[games[gameNO].nr].win=0;
		}
		else if (games[gameNO].WordsInfo[games[gameNO].nr].user1==0){
			if (games[gameNO].WordsInfo[games[gameNO].nr].user2==1){
				games[gameNO].WordsInfo[games[gameNO].nr].win=1;
				games[gameNO].blood.user2--;
			}
			else if (games[gameNO].WordsInfo[games[gameNO].nr].user2==2){
				games[gameNO].WordsInfo[games[gameNO].nr].win=2;
				games[gameNO].blood.user1--;
			}
		}
		else if (games[gameNO].WordsInfo[games[gameNO].nr].user1==1){
			if (games[gameNO].WordsInfo[games[gameNO].nr].user2==0){
				games[gameNO].WordsInfo[games[gameNO].nr].win=2;
				games[gameNO].blood.user1--;
			}
			else if (games[gameNO].WordsInfo[games[gameNO].nr].user2==2){
				games[gameNO].WordsInfo[games[gameNO].nr].win=1;
				games[gameNO].blood.user2--;
			}
		}
		else if (games[gameNO].WordsInfo[games[gameNO].nr].user1==2){
			if (games[gameNO].WordsInfo[games[gameNO].nr].user2==0){
				games[gameNO].WordsInfo[games[gameNO].nr].win=1;
				games[gameNO].blood.user2--;
			}
			else if (games[gameNO].WordsInfo[games[gameNO].nr].user2==1){
				games[gameNO].WordsInfo[games[gameNO].nr].win=2;
				games[gameNO].blood.user1--;
			}
		}
		games[gameNO].nr++;
	}
	else if (listNO==games[gameNO].listNO.user2){
		int orig_nr=games[gameNO].nr;
		games[gameNO].WordsInfo[games[gameNO].nr].user2=recvdata.attack;
		games[gameNO].u2ok=1;
		int timer=0;
		while (orig_nr==games[gameNO].nr){
			/*timeout attack 0!!*/
		/*	if (timer>9000000){
				games[gameNO].WordsInfo[games[gameNO].nr].user1=0;
				orig_nr=games[gameNO].nr;
			}
			timer++;*/
		}
		games[gameNO].u2ok=0;
		return;
	}
	/*send attack*/
	SendData sd1,sd2;
	memset(&sd1, 0, sizeof(SendData));
	memset(&sd2, 0, sizeof(SendData));
	sd1.type=sd2.type=4;
	sd1.state=sd2.state=1;

	strncpy(sd1.username,list[listNO].name,NAMELEN);
	strncpy(sd2.username,list[listNO].enemyname,NAMELEN);

	strncpy(sd1.challenger,list[listNO].enemyname,NAMELEN);
	strncpy(sd2.challenger,list[listNO].name,NAMELEN);

	sd1.blood_value=games[gameNO].blood.user1;
	sd2.blood_value=games[gameNO].blood.user2;

	sd1.attack=games[gameNO].WordsInfo[games[gameNO].nr-1].user1;
	sd2.attack=games[gameNO].WordsInfo[games[gameNO].nr-1].user2;
	
	sd1.fight_back=games[gameNO].WordsInfo[games[gameNO].nr-1].user2;
	sd2.fight_back=games[gameNO].WordsInfo[games[gameNO].nr-1].user1;

	/*who win this attack*/
	if (games[gameNO].WordsInfo[games[gameNO].nr-1].win==1){
		sd1.win=1;
		sd2.win=0;
	}
	else if (games[gameNO].WordsInfo[games[gameNO].nr-1].win==2){
		sd1.win=0;
		sd2.win=1;
	}
	else{
		sd1.win=sd2.win=2;
	}	

	if (send(list[games[gameNO].listNO.user1].connfd,&sd1,SENDMSGLEN,0)<0){
		perror("Send error(askChal) ");
	}	

	if (send(list[games[gameNO].listNO.user2].connfd,&sd2,SENDMSGLEN,0)<0){
		perror("Send error(askChal) ");
	}	
	printf("Playing: Thread ID: %ld PK %ld  name: %s PK %s\n",list[games[gameNO].listNO.user1].pid,list[games[gameNO].listNO.user2].pid,list[games[gameNO].listNO.user1].name,list[games[gameNO].listNO.user2].name);
	/*this challenge is or not over*/
	if (games[gameNO].blood.user1==0||games[gameNO].blood.user2==0){
		/*TODO lock signal_list*/
		while (signal_list);
		SD.type=10;
		strncpy(SD.username,list[games[gameNO].listNO.user1].name,NAMELEN);
		strncpy(SD.challenger,list[games[gameNO].listNO.user2].name,NAMELEN);
		if (games[gameNO].blood.user1==0){
			list[games[gameNO].listNO.user1].score--;
			list[games[gameNO].listNO.user2].score++;
		}
		else if (games[gameNO].blood.user2==0){
			list[games[gameNO].listNO.user1].score++;
			list[games[gameNO].listNO.user2].score--;
		}
		SD.user_score=htonl(list[games[gameNO].listNO.user1].score);
		SD.challenger_score=htonl(list[games[gameNO].listNO.user2].score);
		signal_list=1;
		list[games[gameNO].listNO.user1].state=1;
		list[games[gameNO].listNO.user2].state=1;
		/*TODO lock signal_list*/
	}

}

void *recv_send(void *fp){
	long pid=t;
	int connfd=(int)fp;
	RecvData recvdata;
	while(true){
		//receive
		int rc=recv(connfd,&recvdata,RECVMSGLEN,0);
		if (rc<0){
			perror("Receive error: ");
		}
		else if (rc==0){
			close(connfd);
			printf("The thread %ld end.\n",pid);
			return;
		}
		else if (rc==0)
			return;

		//parse recv socket
		short listNO;
		switch (recvdata.type){
			case 1: 
				listNO=userLogin(recvdata,connfd,pid); 
				int connfd=(int)fp;

				pthread_t thread2;
				int rc;

				rc=pthread_create(&thread2,NULL,listenChalask,(void *)connfd);
				if (rc){
					printf("ERROR!");
					exit(-1);
				}
				break;
			case 2: 
				userExit(recvdata); 
				return;
			case 3: askChal(listNO,recvdata); break;
			case 4: playing(listNO,recvdata); break;
			case 5:case 6: replyChal(listNO,recvdata);break;
			default: assert(0);
		}
	}
}

int main(){
	init();
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

	pthread_t threads[MAXPLAYER], thread1;
	int rc;
	/*send msg for every pthread*/
	rc=pthread_create(&thread1,NULL,listenUserlist,NULL);
	if (rc){
		printf("ERROR!");
		exit(-1);
	}
	long t1;
	for(t1=0;t1<MAXPLAYER;t1++){
		int connfd=accept(listenfd,(struct sockaddr*) &cliaddr, &clilen);
		printf("Creating thread %ld\n",t1);
		/*let send socket send one by one*/
		setsockopt(connfd,IPPROTO_TCP,TCP_NODELAY,NULL,0);
		rc=pthread_create(&threads[t1],NULL,recv_send,(void *)connfd);
		t=t1;
		if (rc){
			printf("ERROR!");
			exit(-1);
		}
	}
	return 1;
}


