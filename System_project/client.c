#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <netinet/in.h>
#include <gtk/gtk.h>

#define BUF_SIZE 256
#define NAME_SIZE 100
#define MSG_SIZE 256 // 한번에 읽을 수 있는 메세지 수

struct MSG {
	char name[NAME_SIZE];
	char msg[MSG_SIZE];
	char file[MSG_SIZE];
	char cmd[MSG_SIZE];
};
struct MSG user_msg;

void * send_msg(void * arg);
void * recv_msg(void * arg);
void error_handling(char * msg);
void clearBuffer(void);
void ftp_list(char *path);

// char name[NAME_SIZE]="[ GUEST ]";
char msg[MSG_SIZE];
char file_path[BUF_SIZE];
pthread_mutex_t mutx;

int main(int argc, char *argv[])
{
	int sock;
	struct sockaddr_in serv_addr;
	pthread_t snd_thread, rcv_thread;
	void * thread_return;

	if(argc!=2) {
		printf("Please Input USER ID! \n");
		exit(1);
	 }

	sprintf(user_msg.name, "[ %s ] ", argv[1]);
	sock=socket(PF_INET, SOCK_STREAM, 0);

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
	serv_addr.sin_port=htons(9743);

	if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1)
		error_handling("connect() error");

		pthread_create(&snd_thread, NULL, send_msg, (void*)&sock);
		pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock);
		pthread_join(snd_thread, &thread_return);
		pthread_join(rcv_thread, &thread_return);

	close(sock);

	return 0;
}

/*서버로부터 받은 메세지 출력*/
void * recv_msg(void * arg)
{
	int sock = *((int*)arg);
	//char name_msg[NAME_SIZE + BUF_SIZE];
	int str_len, i;
	int clnt_num = 0;

	while (1)
	{
		str_len = read(sock, (struct MSG *)&user_msg, sizeof(user_msg));
		if (str_len == -1)
			return (void*)-1;

		// msg[str_len] = 0;
		printf("%s", user_msg.msg);
	}
	return NULL;
}

/*서버에게 메세지 전송*/
void * send_msg(void * arg)
{
	int sock=*((int*)arg);
	char name_msg[NAME_SIZE + BUF_SIZE];
	int str_len = 0;
	char clnt_msg[MSG_SIZE];
	char serv_msg[MSG_SIZE];
	char file_cmd[BUF_SIZE];
	char file_list_path[BUF_SIZE]; 	// 파일 리스트 출력 경로 입력
	int judge = 0; // 파일 업로드 다운로드 구분

	printf("%s 님이 입장 하셨습니다.!! \n", user_msg.name);
	printf("명령어 모음 : @list, @bye, @stop, @continue @file\n");
	//sprintf(name_msg, "%s", name);
	//write(sock, name_msg, strlen(name_msg));
	write(sock, (void *)&user_msg, sizeof(user_msg));

	while(1){
		printf("Input MSG : ");
		fgets(msg, BUF_SIZE, stdin);
		//printf("receieve : %s %d", msg, strcmp(msg, "@bye\n"));
		//printf("\nbye : %d | list : %d | stop : %d | continue : %d | file : %d "
		//, strcmp(msg, "@bye\n"), strcmp(msg, "@list\n"), strcmp(msg, "@stop\n"), strcmp(msg, "@continue\n")
		//, strcmp(msg, "@ftp\n"));

		if(!strcmp(msg,"@bye\n"))
		{
			sprintf(user_msg.msg, "%s", msg);
			write(sock, (void *)&user_msg, sizeof(user_msg));
			close(sock);
			exit(0);
		}
		else if(!strcmp(msg,"@list\n")){
			sprintf(user_msg.cmd, "%s", msg);
			write(sock, (void *)&user_msg, sizeof(user_msg));
			read(sock, (struct MSG *)&user_msg, sizeof(user_msg));
			printf("%s\n", user_msg.msg);
		}
		else if(!strcmp(msg,"@continue\n")){
			sprintf(user_msg.cmd, "%s", msg);
			write(sock, (void *)&user_msg, sizeof(user_msg));
		}
		else if(!strcmp(msg,"@stop\n")){
			sprintf(user_msg.cmd, "%s", msg);
			printf("%s \n", user_msg.cmd);
			write(sock, (void *)&user_msg, sizeof(user_msg));
		}
		else if(!strcmp(msg, "@ftp\n")){
			printf("upload or download or list(1 / 2 / 3) : ");
			scanf("%d", &judge);

			if(judge == 1){
				printf("upload file : ");
				scanf("%s", user_msg.file);
			}
			else if(judge == 2){
				printf("upload file : ");
				scanf("%s", user_msg.file);
			}
			else if(judge == 3){
				printf("list path input : ");
				scanf("%s", file_list_path);
				sprintf(file_cmd, "./gtk %s &", file_list_path);
				system(file_cmd);	
				continue;			
			}

			sprintf(file_cmd, "./ftp.sh %d %s &", judge, user_msg.file);
			sprintf(user_msg.cmd, "%s", msg);
			sprintf(user_msg.file, "%s", user_msg.file);
			
			write(sock, (void *)&user_msg, sizeof(user_msg));
			system(file_cmd);
		}
		else{
			sprintf(user_msg.msg, "%s>>%s ", user_msg.name, msg);
			write(sock, (void *)&user_msg, sizeof(user_msg));
		}
	}
	return NULL;
}

void error_handling(char *msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}
