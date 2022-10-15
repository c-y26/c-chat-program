#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>  
#include <time.h>

#define MAXCLIENTS 5
char username[MAXCLIENTS+1][500];

int s3(int sock, int p_sock[],int k, fd_set rfds, struct sockaddr_in clt);
int s4(int sock, int p_sock[], int k, struct sockaddr_in clt);
int s5(int p_sock[], int k);
int s6(int i, int k, int p_sock[]);

int main(){
    int sock;
	int p_sock[MAXCLIENTS+1];
    int reuse;
    struct sockaddr_in svr;
	struct sockaddr_in clt;
    int k; 				// 参加クライアント数
	int s2 = 0;
	fd_set rfds; 
	struct timeval tv;
	int i;
	int max_sock;

    /* ソケットの生成 */
	if((sock=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP))<0) {
		perror("socket");
		exit(1);
	}
	reuse = 1;
	if(setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse))<0) {
		perror("setsockopt");
		exit(1);
	}
	bzero(&svr,sizeof(svr));
	svr.sin_family=AF_INET;
	svr.sin_addr.s_addr=htonl(INADDR_ANY);    
	svr.sin_port=htons(10140);                
    if(bind(sock,(struct sockaddr *)&svr,sizeof(svr))<0) {
		perror("bind");
		exit(1);
	}
    if(listen(sock,5)<0) {       
		perror("listen");
		exit(1);
	}
    k = 0;
	s2 = 1;

    /* 入力待ち */
	if(s2 == 1){
		while(1){
			FD_ZERO(&rfds);       		 
			FD_SET(sock,&rfds);  		 
			for(i=0; i<=k; i++){        
				FD_SET(p_sock[i],&rfds);
			}
			max_sock = sock;
			for(i=0; i<=k; i++){
				if(max_sock <= p_sock[i]){
					max_sock = p_sock[i];
				}
			}
			tv.tv_sec = 1;
			tv.tv_usec = 0;

			if(select(max_sock+1,&rfds,NULL,NULL,&tv)>0) {
				k = s3(sock,p_sock,k,rfds,clt);
			}
		
		}
	}
    
}

/* ソケットに入力があった場合の処理 */
int s3(int sock, int p_sock[], int k, fd_set rfds, struct sockaddr_in clt){
	if(FD_ISSET(sock,&rfds)) {  
	 	k = s4(sock,p_sock,k,clt);  
	}
	for(int i=0; i<=k; i++){
		if(FD_ISSET(p_sock[i],&rfds)){
			k = s6(i,k,p_sock);  
		}
	}
	return k;
}

/* ソケットの参加受け付け */
int s4(int sock, int p_sock[], int k, struct sockaddr_in clt){
	int newsock;
	int clen = sizeof(clt);
	if((newsock = accept(sock,(struct sockaddr *)&clt,&clen))<0) {
		perror("accept");
		exit(2);
	}
	if(k < MAXCLIENTS){
		char buf[] = "REQUEST ACCEPTED\n"; 
		write(newsock,buf,17);
		p_sock[k+1] = newsock;
		k = s5(p_sock,k);
	} else {
		char buf[] = "REQUEST REJECTED\n";
		write(newsock,buf,17);
		close(newsock);
	}

	return k;
}

/* ユーザ名登録 */
int s5(int p_sock[],int k){
	int rbytes;
	char rbuf[1024];
	if((rbytes = read(p_sock[k+1],rbuf,sizeof(rbuf)))<0) {
		perror("read");
	} else if(rbytes == 0) {
		exit(0);
	} else {
		int i=1;
		char user[1024];
		snprintf(user, rbytes+1, "%s", rbuf);
		while((strcmp(username[i],user) != 0) && (i<=k)){
			i++;
		}
		if(i <= k){
			char message[] = "USERNAME REJECTED\n";
			write(p_sock[k+1],message,18);
			char s[] = "can't resister name : ";
			strncat(s, rbuf, rbytes);
			write(1,s,rbytes+22); 
			close(p_sock[k+1]);
		} else{
			snprintf(username[k+1], rbytes+1, "%s", rbuf);
			char message[] = "USERNAME REGISTERED\n";
			write(p_sock[k+1],message,20);
			char s[] = "resistered name : ";
			strncat(s, rbuf, rbytes);
			write(1,s,rbytes+18); 
			k += 1;
		}

	}
	return k;
}

/* メッセージ配信 */
int s6(int i,int k,int p_sock[]){
	int rbytes;
	char rbuf[512];
	int s7 = 0;
	char user[1024];
	char message[1024];
	if((rbytes = read(p_sock[i],rbuf,sizeof(rbuf)-1)) < 0) {
		perror("read");
	} else if(rbytes == 0){
		s7 = 1;
	} else {
		snprintf(user, strlen(username[i]), "%s",username[i]);
		snprintf(message, strlen(user)+5, "%s >> ", user);
		strncat(message, rbuf, rbytes);
		for(int j=1; j<=k; j++){
			write(p_sock[j], message, strlen(message)+1);
		}
	}
	/* 離脱処理 */
	if(s7 == 1){
		close(p_sock[i]);
		char name[] = "log out : ";
		strncat(name, username[i], strlen(username[i]));
		write(1, name, strlen(username[i])+10);
		if(i < k){
			for(int j=i; j<k; j++){
				char w[1024];
				int s;
				snprintf(w, strlen(username[j+1])+1, "%s", username[j+1]);
				s = p_sock[j+1];
				snprintf(username[j], strlen(w)+1, "%s", w);
				p_sock[j] = s;
			}
		} 
		k -= 1;
	}
	
	return k;
}