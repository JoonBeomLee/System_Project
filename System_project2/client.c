#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define MAXLINE     1000
#define NAME_LEN    20

char *EXIT_STRING = "@bye";
char *LIST = "@list";
char *FTP = "@list";
char *HELP = "@help";

// send read 로 교환될 패킷 구조체
struct PACKET{
	char name[NAME_LEN];
	char msg[MAXLINE];
	char file_name[MAXLINE];
	char cmd[MAXLINE];
};
struct PACKET packet;

// 소켓 생성 및 서버 연결, 생성된 소켓리턴
int tcp_connect(int af, char *servip, unsigned short port);
void errquit(char *mesg) { perror(mesg); exit(1); }

int main(int argc, char *argv[]) {
	//char bufname[NAME_LEN];	// 이름
	char myName[NAME_LEN]; 		// 본인 이름
	char bufmsg[MAXLINE];			// 사용자 입력 메세지
	char bufall[MAXLINE + NAME_LEN];
	int maxfdp1;	// 최대 소켓 디스크립터
	int socket;		// 소켓
	int namelen;	// 이름의 길이

	fd_set read_fds;	// 소켓 위함
	time_t ct;				// 시간 출력 위함
	struct tm tm;			// 시간 출력 위함

	// client 실행 방법 3개 인자 필요
	// EX) ./client server_ip(210.115.229.91), port(임의 약속 값), name(사용될 유저 ID);
	if (argc != 4) {
		printf("사용법 : %s sever_ip  port name \n", argv[0]);
		exit(0);
	}

	// ip, port 사용자 입력 받음
	socket = tcp_connect(AF_INET, argv[1], atoi(argv[2]));
	if (socket == -1)
		errquit("tcp_connect fail");

	puts("서버에 접속되었습니다.");
	maxfdp1 = socket + 1;
	FD_ZERO(&read_fds);		// 소켓 통신전 초기화

	while (1) {
		FD_SET(0, &read_fds);		// 소켓 통신전 초기화
		FD_SET(socket, &read_fds); 	// 소켓 통신전 초기화
		// 소켓 통신 오류 처리, 잘못된 입력
		if (select(maxfdp1, &read_fds, NULL, NULL, NULL) < 0)
			errquit("select fail");
		// 소켓 통신 정상 입력시 내용 출력
		if (FD_ISSET(socket, &read_fds)) {
			//int nbyte;
			// packet 으로 입력 받음
			if ((recv(socket, (struct PACKET *)&packet, sizeof(packet), 0)) > 0) {
				// bufmsg[nbyte] = 0;
				packet.msg[strlen(packet.msg) + 1] = 0;
				write(1, "\033[0G", 4);			//커서의 X좌표를 0으로 이동
				printf("%s", packet.msg);			//메시지 출력
				//fprintf(stderr, "\033[1;32m");		//글자색을 녹색으로 변경
				fprintf(stderr, "%s>", argv[3]);	//내 닉네임 출력
			}
		}
		if (FD_ISSET(0, &read_fds)) {
			if (fgets(bufmsg, MAXLINE, stdin)) {// 사용자 입력 받음
				//fprintf(stderr, "\033[1;33m"); 		//글자색을 노란색으로 변경
				fprintf(stderr, "\033[1A"); 			//Y좌표를 현재 위치로부터 -1만큼 이동
				ct = time(NULL);	//현재 시간을 받아옴
				tm = *localtime(&ct);
				sprintf(packet.msg, "[%02d:%02d:%02d]%s>%s", tm.tm_hour, tm.tm_min, tm.tm_sec, argv[3], bufmsg);//메시지에 현재시간 추가

				// 명령어 처리 client -> server
				//if (send(socket, bufall, strlen(bufall), 0) < 0)
				if(strlen(bufmsg) < 0)
					puts("Error : Write error on socket.");

				/**/
				if (strstr(bufmsg, EXIT_STRING) != NULL) {
					puts("Good bye.\n");
					close(socket);
					exit(0);
				}
				else if(strstr(bufmsg, LIST) != NULL) {
					sprintf(packet.cmd, "%s", LIST);
					send(socket, (void *)&packet, sizeof(packet), 0);
					recv(socket, (struct PACKET *)&packet, sizeof(packet), 0);
					write(1, "\033[0G", 4);			//커서의 X좌표를 0으로 이동
					printf("%s", packet.msg);		//메시지 출력
					sprintf(packet.msg, " ");
					continue;
				}
				else if(strstr(bufmsg, FTP) != NULL){
						send(socket, (void *)&packet, sizeof(packet), 0);
				}

				send(socket, (void *)&packet, sizeof(packet), 0);
			}
		}
	} // end of while
}

// 연결 방식, ip, 포트 입력받음
int tcp_connect(int af, char *servip, unsigned short port) {
	struct sockaddr_in servaddr;
	int  s;
	// 소켓 생성
	if ((s = socket(af, SOCK_STREAM, 0)) < 0)
		return -1;
	// 채팅 서버의 소켓주소 구조체 servaddr 초기화
	bzero((char *)&servaddr, sizeof(servaddr));
	servaddr.sin_family = af;
	inet_pton(AF_INET, servip, &servaddr.sin_addr);
	servaddr.sin_port = htons(port);

	// 연결요청
	if (connect(s, (struct sockaddr *)&servaddr, sizeof(servaddr))
		< 0)
		return -1;
	return s;
}
