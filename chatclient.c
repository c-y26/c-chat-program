#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>   
#include <sys/socket.h> 
#include <netinet/in.h>  
#include <netdb.h>      

void c5(int sock);
void exception(int sock);

int main(int argc,char **argv)
{
    int sock;
    int reuse;
    struct sockaddr_in host;
    struct hostent *hp;
    int rbytes;				// 参加可否(接続受理)のメッセージの長さ
    char rbuf[1024]; 		// 参加可否のメッセージを受信
    int ubytes;      		// ユーザ名の長さ
    char username[1024];  	// ユーザ名
    int urbytes; 			// ユーザ名可否のメッセージの長さ
    char urbuf[1024];    	// ユーザ名可否のメッセージを受信
    fd_set rfds;  
	struct timeval tv;
    int bytes;
    char buf[1024];
    int c3 = 0;
    int c4 = 0;

	if(argc != 3) {            
		fprintf(stderr, "Usaage: %s hostname username\n", argv[0]);
		exit(1);
	}

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
   
	bzero(&host,sizeof(host));
	host.sin_family = AF_INET;
	host.sin_port = htons(10140);   
	if((hp = gethostbyname(argv[1])) == NULL) {
		fprintf(stderr, "unknown host %s\n",argv[1]);
		exit(1);
	}
	
	bcopy(hp->h_addr, &host.sin_addr, hp->h_length); 

    /* ホストへ接続要求  */
	if((connect(sock,(struct sockaddr*)&host,sizeof(host))) < 0) {
		perror("client: connect");
	} else {
		printf("connected to %s\n",argv[1]);
	}

    /* チャットへの参加 */
    if((rbytes = read(sock,rbuf,sizeof(rbuf))) <= 0) { 
		exception(sock);
	} else {
        if((rbytes == 17) && (strncmp(rbuf, "REQUEST ACCEPTED\n",17) == 0)){
			printf("join request accepted\n");
            c3 = 1;
        } else {
			printf("request rejected\n"); 
            exception(sock);
        }
	}

    /* ユーザ名登録 */
    if(c3 == 1){
        strcpy(username, argv[2]);  
        strcat(username, "\n");  
        ubytes = strlen(username);  
        write(sock,username,ubytes);
        if((urbytes = read(sock,urbuf,sizeof(urbuf))) <= 0) { 
		    exception(sock);
	    } else {
            if((urbytes == 20) && (strncmp(urbuf, "USERNAME REGISTERED\n",20) == 0)){
				printf("user name registered\n");
                c4 = 1;
            } else {
				printf("user name unregistered\n");
                exception(sock);
            }
	    }
    }

    /* メッセージ送受信 */
    if(c4 == 1){
        do {
		    FD_ZERO(&rfds);
		    FD_SET(0,&rfds);
		    FD_SET(sock,&rfds);
		    tv.tv_sec = 1;
		    tv.tv_usec = 0;
		
		    if(select(sock+1,&rfds,NULL,NULL,&tv) > 0) {
			    if(FD_ISSET(0,&rfds)) {  
				    if((bytes = read(0,buf,sizeof(buf))) < 0) {
					    perror("read");
					    exit(1);
				    } else if(bytes == 0) {
					    c5(sock);
				    } else {
					    write(sock,buf,bytes);
				    }
			    }
			    if(FD_ISSET(sock,&rfds)) {
				    if((bytes = read(sock,buf,sizeof(buf))) < 0) {
					    perror("read");
					    exit(1);
				    } else if(bytes == 0) {
					    c5(sock);
				    } else {
					    write(1,buf,bytes);
				    }
			    }
		    }
	    } while(1);
    }

	
}

/* 離脱 */
void c5(int sock){
    close(sock);
    exit(0);
}

/* 例外処理 */
void exception(int sock){
    close(sock);
    exit(1);
}