#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <signal.h>

#define MAXLINE  1000
#define MAX_SOCK 1024
#define BUFSIZE 512
#define NAME_LEN 20

// send read 로 교환될 패킷 구조체
struct PACKET{
	char name[NAME_LEN];
	char msg[MAXLINE];
	char file_name[MAXLINE];
	char cmd[MAXLINE];
};
struct PACKET packet;

char *EXIT_STRING = "exit";	// 클라이언트의 종료요청 문자열
char *START_STRING = "Connected to chat_server \n";
char *LIST = "@list";
char *FTP = "@list";
char *HELP = "@help";

// 클라이언트 환영 메시지
int maxfdp1;				// 최대 소켓번호 +1
int num_user = 0;			// 채팅 참가자 수
int num_chat = 0;			// 지금까지 오간 대화의 수
int clisock_list[MAX_SOCK];		// 채팅에 참가자 소켓번호 목록
char ip_list[MAX_SOCK][20];		//접속한 ip목록
char clnt_id[512];						// 접속한 사용자 id를 저장
int listen_sock;			// 서버의 리슨 소켓

							// 새로운 채팅 참가자 처리
void addClient(int s, struct sockaddr_in *newcliaddr);
int getmax();				// 최대 소켓 번호 찾기
void removeClient(int s);	// 채팅 탈퇴 처리 함수
int tcp_listen(int host, int port, int backlog); // 소켓 생성 및 listen
void errquit(char *mesg) { perror(mesg); exit(1); }

time_t ct;
struct tm tm;

void *thread_function(void *arg) { //명령어를 처리할 스레드
	int i;
	printf("명령어 목록 : @help, @num_user, @ip_list\n");
	while (1) {
		char bufmsg[MAXLINE + 1];
		//fprintf(stderr, "\033[1;32m"); //글자색을 녹색으로 변경
		printf("server>"); //커서 출력
		fgets(bufmsg, MAXLINE, stdin); //명령어 입력
		if (!strcmp(bufmsg, "\n")) continue;   //엔터 무시
		else if (!strcmp(bufmsg, "help\n"))    //명령어 처리
			printf("help, num_user, num_chat, ip_list\n");
		else if (!strcmp(bufmsg, "num_user\n"))//명령어 처리
			printf("현재 참가자 수 = %d\n", num_user);
		else if (!strcmp(bufmsg, "ip_list\n")) //명령어 처리
			for (i = 0; i < num_user; i++)
				printf("%s\n", ip_list[i]);
		else //예외 처리
			printf("해당 명령어가 없습니다.help를 참조하세요.\n");
	}
}

void sighandler(int signo) {
	int i;
	char buf[31] = "The server has been shut down\n";
	printf("\nSystem 비정상 종료...\n");
	sprintf(packet.cmd, "@END");
	sprintf(packet.msg, "\n===== System 비정상 종료 =====\n");
	// 모든 채팅 참가자에게 메시지 방송
	for (i = 0; i < num_user; i++)
		send(clisock_list[i], (void *)&packet, sizeof(packet), 0);
	exit(1);
}

int main(int argc, char *argv[]) {
	struct sockaddr_in cliaddr;
	char buf[MAXLINE + 1]; //클라이언트에서 받은 메시지
	int i, j, nbyte, accp_sock, addrlen = sizeof(struct
		sockaddr_in);
	fd_set read_fds;	//읽기를 감지할 fd_set 구조체
	pthread_t a_thread;

	if (argc != 2) {
		printf("사용법 :%s port\n", argv[0]);
		exit(0);
	}

	signal(SIGINT, (void *)sighandler); // signal handler for SIGINT

	// tcp_listen(host, port, backlog) 함수 호출
	listen_sock = tcp_listen(INADDR_ANY, atoi(argv[1]), 5);
	//스레드 생성
	pthread_create(&a_thread, NULL, thread_function, (void *)NULL);
	while (1) {
		FD_ZERO(&read_fds);
		FD_SET(listen_sock, &read_fds);
		for (i = 0; i < num_user; i++)
			FD_SET(clisock_list[i], &read_fds);

		maxfdp1 = getmax() + 1;	// maxfdp1 재 계산
		if (select(maxfdp1, &read_fds, NULL, NULL, NULL) < 0)
			errquit("select fail");

		if (FD_ISSET(listen_sock, &read_fds)) {
			accp_sock = accept(listen_sock,
				(struct sockaddr*)&cliaddr, &addrlen);
			if (accp_sock == -1) errquit("accept fail");
			// 클라이언트 추가 문
			addClient(accp_sock, &cliaddr);

			//sprintf(packet.msg, "%s", START_STRING);
			send(accp_sock, (void *)&packet, sizeof(packet), 0);
			ct = time(NULL);			//현재 시간을 받아옴
			tm = *localtime(&ct);
			write(1, "\033[0G", 4);		//커서의 X좌표를 0으로 이동
			printf("[%02d:%02d:%02d]", tm.tm_hour, tm.tm_min, tm.tm_sec);
			//fprintf(stderr, "\033[33m");//글자색을 노란색으로 변경
			printf("사용자 1명 추가. 현재 참가자 수 = %d\n", num_user);
			//fprintf(stderr, "\033[32m");//글자색을 녹색으로 변경
			//fprintf(stderr, "server>"); //커서 출력
		}

		// 클라이언트가 보낸 메시지를 모든 클라이언트에게 방송
		for (i = 0; i < num_user; i++) {
			if (FD_ISSET(clisock_list[i], &read_fds)) {
				num_chat++;				//총 대화 수 증가
				int nbyte;

				nbyte = recv(clisock_list[i], (struct PACKET *)&packet, sizeof(packet), 0);
				printf("\033[0G");		//커서의 X좌표를 0으로 이동
				//fprintf(stderr, "\033[97m"); //글자색을 흰색으로 변경
				printf("%s", packet.msg);			//메시지 출력
				//fprintf(stderr, "\033[32m"); //글자색을 녹색으로 변경
				fprintf(stderr, "server>"); //커서 출력
				
				if (nbyte <= 0) {
					removeClient(i);	// 클라이언트의 종료
					continue;
				}

				// 종료문자 처리
				if (strstr(packet.cmd, EXIT_STRING) != NULL) {
					removeClient(i);	// 클라이언트의 종료
					continue;
				}
				else if (strstr(packet.cmd, LIST) != NULL) {
					sprintf(packet.msg, "==== Connected Client List ====\n");
					send(clisock_list[i], (void *)&packet, sizeof(packet), 0);
					sprintf(packet.msg, " ");
					continue;
				}


				// 모든 채팅 참가자에게 메시지 방송
				for (j = 0; j < num_user; j++){
					send(clisock_list[j], (void *)&packet, sizeof(packet), 0);
				}


			}
		}

	}  // end of while

	return 0;
}

// 새로운 채팅 참가자 처리
void addClient(int s, struct sockaddr_in *newcliaddr) {
	char buf[20];
	inet_ntop(AF_INET, &newcliaddr->sin_addr, buf, sizeof(buf));
	write(1, "\033[0G", 4);		//커서의 X좌표를 0으로 이동
	//fprintf(stderr, "\033[33m");	//글자색을 노란색으로 변경
	printf("new client: %s\n", buf);//ip출력
	// 채팅 클라이언트 목록에 추가
	clisock_list[num_user] = s;
	strcpy(ip_list[num_user], buf);
	num_user++; //유저 수 증가
}

// 채팅 탈퇴 처리
void removeClient(int s) {
	close(clisock_list[s]);
	if (s != num_user - 1) { //저장된 리스트 재배열
		clisock_list[s] = clisock_list[num_user - 1];
		strcpy(ip_list[s], ip_list[num_user - 1]);
	}
	num_user--; //유저 수 감소
	ct = time(NULL);			//현재 시간을 받아옴
	tm = *localtime(&ct);
	write(1, "\033[0G", 4);		//커서의 X좌표를 0으로 이동
	//fprintf(stderr, "\033[33m");//글자색을 노란색으로 변경
	printf("[%02d:%02d:%02d]", tm.tm_hour, tm.tm_min, tm.tm_sec);
	printf("채팅 참가자 1명 탈퇴. 현재 참가자 수 = %d\n", num_user);
	//fprintf(stderr, "\033[32m");//글자색을 녹색으로 변경
	//fprintf(stderr, "server>"); //커서 출력
}

// 최대 소켓번호 찾기
int getmax() {
	// Minimum 소켓번호는 가정 먼저 생성된 listen_sock
	int max = listen_sock;
	int i;
	for (i = 0; i < num_user; i++)
		if (clisock_list[i] > max)
			max = clisock_list[i];
	return max;
}

// listen 소켓 생성 및 listen
int  tcp_listen(int host, int port, int backlog) {
	int sd;
	struct sockaddr_in servaddr;

	sd = socket(AF_INET, SOCK_STREAM, 0);
	if (sd == -1) {
		perror("socket fail");
		exit(1);
	}
	// servaddr 구조체의 내용 세팅
	bzero((char *)&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(host);
	servaddr.sin_port = htons(port);
	if (bind(sd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		perror("bind fail");  exit(1);
	}
	// 클라이언트로부터 연결요청을 기다림 대기큐 크기 default = 5; (가장 많이 사용)
	listen(sd, backlog);
	return sd;
}