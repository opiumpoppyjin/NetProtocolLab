#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

#define SERV_PORT 4321 
#define MAXPLAYER 32
#define RECVMSGLEN 68
#define SENDMSGLEN 81
#define SENDLISTLEN 1252
//允许保存的未处理连接请求数量
#define LISTENQ 8

#define bool int 
#define true 1
#define false 0

#define NAMELEN 33

#define TCP_NODELAY 1

#pragma pack(1)
/*recv user info from client*/
typedef struct RecvData{
	char type; //1:login 2:exit 3:challenge 4:attack 5:reply1 6:reply0
	char username[33]; 
	char user2name[33]; 
	char attack; // 出拳方式 0:石头 1:剪刀 2:布
}RecvData;
#pragma pack()


#pragma pack(1)
/*user info send to client*/
typedef struct SendData{
	char type;//5:other login 6:other exit 7:ask 8:query ask 9:playing 10:become free
	char state;	//0:失败 1：成功;对登陆请求，退出请求,对战请求的回应
	char username[33];
	char challenger[33];
	char blood_value;
	char attack;
	char fight_back;	//对手出拳方式
	char win;	//判断用户是赢1还是输0
	char errorNO;
	int user_score;
	int challenger_score;
	//对应于state请求失败的原因 0：用户名已存在 1：请求对战时没有该用户名 2：请求对战时该用户已经处于对战状态 3：请求对战时对方不同意对战 4：请求对战时对方长时间不响应 5:用户列表已满
}SendData;
#pragma pack()
SendData SD;
/*userlist send to client*/
#pragma pack(1)
typedef struct User{
	char username[33];
	char blood_value;
	char state; //0:对战 1:非对战 
	int score;
}User;
#pragma pack()

#pragma pack(1)
typedef struct SendUserlist{
	unsigned int num;
	User userlist[MAXPLAYER];
}SendUserlist;
#pragma pack()

/*server's own userlist*/
struct UserList{
	char state; //0:no user, 1:waiting, 2:playing,
	int connfd;
	unsigned long pid;
	char name[NAMELEN];	
	char enemyname[NAMELEN];
	short gamesNO;
	int score;
}list[MAXPLAYER];

struct GameInfo{
	short use; //0:not be used
	struct{
		short user1;
		short user2;
	}listNO;
	struct{
		char user1;
		char user2;
	}blood; 
	short nr;
	short u2ok;
	struct{
		char user1;
		char user2;
		char win; //0:equel 1:u1 2:u2
	}WordsInfo[10];//nr
}games[MAXPLAYER];

long t;

char signal_list;//1:when any state change
char signal_askChal;
struct {
	int user1;
	int user2;
}chalConnfd;
struct {
	char user1[NAMELEN];
	char user2[NAMELEN];
}chalname;
char signal_replyChal;
char replyOK;






