#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/types.h>
#include<arpa/inet.h>
#include<unistd.h>
/* port we're listening on */
#define SERVERPORT 1573
/*maximum cnnnect number*/
#define BACKLOG 10
/*maximum data buffer*/
#define BUFSIZE 2048

struct client_info
{
	int client_id;
	char name[256];
	int first;
	struct sockaddr_in client_address;
};

int main()
{
	/*master file descriptor list*/
	fd_set master_fds;
	/*temple file descriptor list for select()*/
	fd_set read_fds;
	/*server address*/
	struct sockaddr_in server_addr;
	/*client address*/
	struct sockaddr_in client_addr;
	/*maximum file descriptor number*/
	int max_fd;
	/*listening socket file descriptor*/
	int sockfd;
	/*newly accept()ed socket file descreptor*/
	int newfd;
	/*buffer for saving client data*/
	char data_buf[BUFSIZE];
	char send_buf[BUFSIZE];
	/*number of client data*/
	int nbytes;
	/*for set socket option*/
	int opt;
	opt = SO_REUSEADDR;
    /*lenth of address*/
	int addr_len;
	/*for accept() to use*/
	int size;
	size = sizeof(struct sockaddr);
	/*temple varient*/
	int tmp_i, tmp_j;
	int i,j;
	char cmd[BUFSIZE];
	char copy[BUFSIZE];
	char* token=NULL;
	char* username=NULL;
	char* message=NULL;
	char *delimiter=" ";
	struct client_info clientinfo[BACKLOG];
	/*clear the master and temple file descriptor*/
	FD_ZERO(&master_fds);
	FD_ZERO(&read_fds);
	
	memset(&data_buf, 0, BUFSIZE);
	memset(&send_buf, 0, BUFSIZE);
	memset(&copy,0,BUFSIZE);

	/*create socket*/
	if (-1 == (sockfd = socket(AF_INET, SOCK_STREAM, 0)))
	{
		perror("create socket() error:");
		exit(1);
	}
	/*set the socket*/
    if (-1 == setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
			perror("setsockopt() error:");
			exit(1);
    }
	/*bind first config the socket then binding*/
	memset(&server_addr, 0, size);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVERPORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	bzero(&(server_addr.sin_zero), 8);
	if (-1 == bind(sockfd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr)))
	{
		perror("bind() socket error:");
		exit(1);
	}
	/*listen */
	if (-1 == listen(sockfd, BACKLOG))
	{
		perror("listen() error:");
		exit(1);
	}
	/*add sockfd to master set*/
	FD_SET(sockfd, &master_fds);
	/*keep track the max file descriptor*/
	max_fd = sockfd;
	printf("server is ok!\n");
	/*loop*/
	while (1)
	{
		read_fds = master_fds;
		if (-1== select(max_fd + 1, &read_fds, NULL, NULL, NULL))
		{
			perror("select() error!\n");
			exit(1);
		}
		/*looking for data to read*/
		for (tmp_i = sockfd; tmp_i <= max_fd; tmp_i++)
		{
			/*got connect*/
			if (FD_ISSET(tmp_i, &read_fds))
			{
				if (tmp_i == sockfd)  //如果是监听套接字
				{
					newfd = accept(sockfd, (struct sockaddr*)&client_addr, &size);
					clientinfo[newfd].client_id = newfd;
					clientinfo[newfd].client_address.sin_addr=client_addr.sin_addr;
					clientinfo[newfd].first = 1;
					if (-1 == newfd)
					{
						perror("accept() error:");
						exit(1);
					}
					else
					{
						FD_SET(newfd, &master_fds);
						if (newfd > max_fd)    //找出文件描述符集合中最大的文件描述符
						{
							max_fd = newfd;
						}
						printf("Get the new connect from %s\n", inet_ntoa(client_addr.sin_addr));
					}
				}
				else //如果连接套接字被激活
				{/*get data from the client*/
							nbytes = read(tmp_i, data_buf, sizeof(data_buf));
							if (0 > nbytes)
							{
								perror("recv() error:");
								exit(1);
							}
							else if(clientinfo[tmp_i].first == 1){ //如果是首次登录
								int existed = 0;
								data_buf[nbytes-1]='\0';
								for (tmp_j = sockfd + 1; tmp_j <= max_fd; tmp_j++)
								{
									if (FD_ISSET(tmp_j, &master_fds) && 0==strcmp(clientinfo[tmp_j].name,data_buf))
									{
										existed=1;
										strcat(send_buf,data_buf);
										strcat(send_buf," has already existed, please input a new nickname:");
										if (-1 == write(tmp_i, send_buf, sizeof(send_buf)))
										{
											perror("send data error:");
										}
										break;
									}
								}// end for		
								if(!existed){
									clientinfo[tmp_i].first=0;
									memset(&clientinfo[tmp_i].name,0,256);
									strcpy(clientinfo[tmp_i].name,data_buf);
								}
							}
							else if(nbytes==0 || (data_buf[0]=='/')){
								if(strlen(data_buf)>5){
									memset(cmd,0,BUFSIZE);
									for(i=0;i<5;i++){
										cmd[i]=data_buf[i];
									}
								}
								printf("get command:%s from the client :", data_buf);
								printf("%s\n", inet_ntoa(clientinfo[tmp_i].client_address.sin_addr));
								if(nbytes==0 || (0==strcmp(data_buf,"/quit"))){
									printf("client:  %s %s exit!\n",inet_ntoa(clientinfo[tmp_i].client_address.sin_addr),clientinfo[tmp_i].name) );
									memset(clientinfo[tmp_i].name,0,256);
									FD_CLR(tmp_i, &master_fds);
									close(tmp_i);
									strcat(send_buf,inet_ntoa(clientinfo[tmp_i].client_address.sin_addr) );
									strcat(send_buf,clientinfo[tmp_i].name);
									strcat(send_buf, "  was exit!");
									for (tmp_j = sockfd + 1; tmp_j <= max_fd; tmp_j++)
									{
										if (FD_ISSET(tmp_j, &master_fds))
										{

											if (-1 == write(tmp_j, send_buf, nbytes))
											{
												perror("send data error:");
											}
										}
									}// end for
								}
								else if(0==strcmp(data_buf,"/help")){
									printf(" The help menu has already printed");
								}
								else if(0==strcmp(data_buf,"/who")){
									for(tmp_j = sockfd + 1; tmp_j <= max_fd; tmp_j++){
										if(FD_ISSET(tmp_j, &master_fds) ){
											strcat(send_buf, clientinfo[tmp_j].name);
											strcat(send_buf, " is online.\n");
										}
									}
									if (-1 == write(tmp_i, send_buf, sizeof(send_buf)))
										perror("send data error:");
								}
								else if(strlen(data_buf)==5 && 0==strcmp(data_buf,"/send")){
									strcat(send_buf,"Wrong format,please input /send username message");
									if (-1 == write(tmp_i, send_buf, sizeof(send_buf)))
										perror("send data error:");
								}
								else if(0==strcmp(cmd,"/send")){
									strcpy(copy,data_buf);
									token=strtok(copy,delimiter);
									username=strtok(NULL,delimiter);
									message=strtok(NULL,delimiter);
									message=strstr(data_buf,message);
									int online=0;
									for (tmp_j = sockfd + 1; tmp_j <= max_fd; tmp_j++){
										if (FD_ISSET(tmp_j, &master_fds) && (0==strcmp(clientinfo[tmp_j].name, username)))
										{
											online = 1;
											strcat(send_buf, clientinfo[tmp_i].name);
											strcat(send_buf, " said:  ");
											strcat(send_buf, message);
											if (-1 == write(tmp_j, send_buf, nbytes))
												perror("send data error:");
										}
									}// end for
									if(!online){
										strcpy(send_buf, "user isn't online\n");
										if (-1 == write(tmp_i, send_buf, nbytes))
											perror("send data error:");
									}
								}
								else
								{
									strcat(send_buf, "command not found");
									if (-1 == write(tmp_i, send_buf, sizeof(send_buf)))
										perror("send data error:");
								}//end else
					
							}
							
							else{//非命令
								printf("get data:%s from the client :", data_buf);
								printf("%s\n", inet_ntoa(clientinfo[tmp_i].client_address.sin_addr));
								strcat(send_buf, inet_ntoa(clientinfo[tmp_i].client_address.sin_addr));
								strcat(send_buf, " said:  ");
								strcat(send_buf, data_buf);
								for (tmp_j = sockfd + 1; tmp_j <= max_fd; tmp_j++)
								{
									if (FD_ISSET(tmp_j, &master_fds))
									{
										if (-1 == write(tmp_j, send_buf, nbytes))
											perror("send data error:");
									}
								}// end for
							}//end else
					
				}//end else
			}//end if
		}//end for 

    memset(&data_buf, 0, BUFSIZE);
	memset(&send_buf, 0, BUFSIZE);
	//FD_ZERO(&master_fds);
	//FD_SET(sockfd, &master_fds);
	}//end while

return 0;
}

