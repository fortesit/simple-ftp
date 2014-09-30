/*
 
 Simple FTP
 
 Version: 1.0
 GitHub repository: https://github.com/fortesit/simple-ftp
 Author: Sit King Lok
 Last modified: 2014-09-30 22:11
 
 Description:
 A simple FTP with the following features:
 1. List files (i.e. ls)
 2. Download file (i.e. get [FILENAME])
 3. Upload file (i.e. put [FILENAME])
 4. User login (i.e. auth [USER] [PASSWORD])
 5. Multi-thread(Multi-user) support
 6. Multi-platform support
 
 Required files:
 MakeFile
 myftp.h
 myftpclient.c
 myftpserver.c
 access.txt
 
 Usage:
 make all
 ./client_{linux|unix}
 
 Platform:
 Linux(e.g.Ubuntu)/SunOS
 
 Note:
 You need to have a password file (access.txt) in order to login.
 The sample file provided contain the test account.
 User:alice
 Password:pass1
 
 Example:
 See README.md
 
 */

# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <string.h>
# include <errno.h>
# include <signal.h>
# include <sys/socket.h>
# include <sys/types.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include "myftp.h"

# define DEBUG_MODE 0

int sd = 0;
short conn = 0, auth = 0;
const char myftp_protocol[6] = {0xe3,'m','y','f','t','p'};

void dump_memory(void const* data, size_t len)
{
	if (DEBUG_MODE) {
		size_t i;
		printf("Data in [%p..%p): ", data, data + len);
        for (i = 0;i < len; i++) {
			printf("%02X ", ((unsigned char*)data)[i]);
        }
		printf("\n");
	}
	return;
}

int send_packet(const void* buffer, int length)
{
	int sentLength = 0;
	while (sentLength < length) {
		int len = send(sd, buffer + sentLength, length - sentLength, 0);
		if (len < 0) {
			printf("ERROR: When sending data, %s (Errno:%d)\n", strerror(errno), errno);
			break;
		}
		sentLength += len;
	}
	if (DEBUG_MODE) {
		printf("Send ");
		dump_memory(buffer, length);
	}
	return sentLength;
}

int receive_packet(void* buffer, int length)
{
	int receivedLength = 0;
	while (receivedLength < length) {
		int len = recv(sd, buffer + receivedLength, length - receivedLength, 0);
		if(len < 0){
			printf("ERROR: When receiving data, %s (Errno:%d)\n", strerror(errno), errno);
			break;
		}
		receivedLength += len;
	}
	if (DEBUG_MODE) {
		printf("Receive ");
		dump_memory(buffer, length);
	}
	return receivedLength;
}

int open_cmd(char* server_ip, int server_port)
{
	if (conn == 1) {
		printf("ERROR: You have opened the connection.\n");
		return -1;
	}
	if (server_port == 0) {
		printf("ERROR: Please specify a correct port number.\n");
		return -1;
	}
	sd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(server_ip);
	if (server_addr.sin_addr.s_addr == -1) {
		printf("ERROR: Please specify a correct IP address.\n");
		return -1;
	}
	server_addr.sin_port = htons(server_port);
	if(connect(sd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		printf("ERROR: When connecting the server, %s (Errno:%d)\n", strerror(errno), errno);
		return -1;
	}
    
	// send OPEN_CONN_REQUEST
	struct message_s OPEN_CONN_REQUEST;
	memcpy(OPEN_CONN_REQUEST.protocol, myftp_protocol, 6);
	OPEN_CONN_REQUEST.type = 0xA1;
    
	// OPEN_CONN_REQUEST.status = unused;
	OPEN_CONN_REQUEST.length = htonl(12);
	send_packet(&OPEN_CONN_REQUEST, 12);
    
	// wait and receive OPEN_CONN_REPLY
	struct message_s OPEN_CONN_REPLY;
	receive_packet(&OPEN_CONN_REPLY, 12);
	if (memcmp(OPEN_CONN_REPLY.protocol, myftp_protocol, 6) != 0 || OPEN_CONN_REPLY.type != (char)0xA2 || ntohl(OPEN_CONN_REPLY.length) != 12) {
		printf("ERROR: Received wrong data. Connection closed.\n");
		close(sd);
		return -1;
	}
	if (OPEN_CONN_REPLY.status != 1) {
		printf("ERROR: Server refused to connect. (Errno:%d)\n", OPEN_CONN_REPLY.status);
		return -1;
	}
    
	printf("Server connection accepted.\n");
	return 1;
}

int auth_cmd(void* payload)
{
	if (conn != 1) {
		printf("ERROR: You did not open any connection. Please issus an 'open' command.\n");
		return -1;
	}
	if (auth == 1) {
		printf("ERROR: You have be granted authentication.\n");
		return -1;
	}
    
	// send AUTH_REQUEST
	struct message_s AUTH_REQUEST;
	memcpy(AUTH_REQUEST.protocol, myftp_protocol, 6);
	AUTH_REQUEST.type = 0xA3;
    
	// AUTH_REQUEST.status = unused;
	AUTH_REQUEST.length = htonl(12 + strlen(payload) + 1);
	send_packet(&AUTH_REQUEST, 12);
	send_packet(payload, strlen(payload) + 1);
	
    // wait and receive AUTH_REPLY
	struct message_s AUTH_REPLY;
	receive_packet(&AUTH_REPLY, 12);
	if (memcmp(AUTH_REPLY.protocol, myftp_protocol,6) != 0 || AUTH_REPLY.type != (char)0xA4 || ntohl(AUTH_REPLY.length) != 12) {
		printf("ERROR: Received wrong data. Connection closed.\n");
		close(sd);
		conn = 0;
		return -1;
	}
	if (AUTH_REPLY.status == 0) {
		printf("ERROR: Authentication rejected. Connection closed.\n");
		conn = 0;
		close(sd);
		return -1;
	}
	printf("Authentication granted.\n");
    
	return 1;
}

int ls_cmd()
{
	if (conn != 1) {
		printf("ERROR: You did not open any connection.\n");
		return -1;
	}
	if (auth != 1) {
		printf("ERROR: You were not granted authentication.\n");
		return -1;
	}
    
	// send LIST_REQUEST
	struct message_s LIST_REQUEST;
	memcpy(LIST_REQUEST.protocol, myftp_protocol, 6);
	LIST_REQUEST.type = 0xA5;
    
	// LIST_REQUEST.status = unused;
	LIST_REQUEST.length = htonl(12);
	send_packet(&LIST_REQUEST, 12);
    
	// wait and receive LIST_REPLY
	struct message_s LIST_REPLY;
	receive_packet(&LIST_REPLY, 12);
	if (memcmp(LIST_REPLY.protocol, myftp_protocol, 6) != 0 || LIST_REPLY.type != (char)0xA6 || ntohl(LIST_REPLY.length) < 13) {
		printf("ERROR: Received wrong data. Connection closed.\n");
		close(sd);
		conn = 0;
		return -1;
	}
	int len_of_payload = ntohl(LIST_REPLY.length) - 12;
	void *payload = malloc(len_of_payload);
	receive_packet(payload, len_of_payload);
	printf("---- %s ----\n", "file list start");
	printf("%s", (char*)payload);
	printf("---- %s ----\n", "file list end");
    
	return 1;
}

int get_cmd(void* payload)
{
	if (conn != 1) {
		printf("ERROR: You did not open any connection.\n");
		return -1;
	}
	if (auth != 1) {
		printf("ERROR: You were not granted authentication.\n");
		return -1;
	}
    
	// send GET_REQUEST
	struct message_s GET_REQUEST;
	memcpy(GET_REQUEST.protocol, myftp_protocol, 6);
	GET_REQUEST.type = 0xA7;
    
	// GET_REQUEST.status = unused;
	GET_REQUEST.length = htonl(12 + strlen(payload) + 1);
	send_packet(&GET_REQUEST, 12);
	send_packet(payload, strlen(payload) + 1);
    
	// wait and receive GET_REPLY
	struct message_s GET_REPLY;
	receive_packet(&GET_REPLY, 12);
	if (memcmp(GET_REPLY.protocol, myftp_protocol, 6) != 0 || GET_REPLY.type != (char)0xA8 || ntohl(GET_REPLY.length) != 12) {
		printf("ERROR: Received wrong data. Connection closed.\n");
		close(sd);
		conn = 0;
		return -1;
	}
	if (GET_REPLY.status == 0) {
		printf("ERROR: The file is not existed.\n");
		return -1;
	}
    
	// wait and receive FILE_DATA
	struct message_s FILE_DATA;
	receive_packet(&FILE_DATA, 12);
	int len_of_payload = ntohl(FILE_DATA.length) - 12;
	void *file_payload = malloc(len_of_payload);
	receive_packet(file_payload, len_of_payload);
	if(memcmp(FILE_DATA.protocol, myftp_protocol, 6) != 0 || FILE_DATA.type != (char)0xFF || ntohl(FILE_DATA.length) < 12) {
		printf("ERROR: Received wrong data. Connection closed.\n");
		close(sd);
		conn = 0;
		return -1;
	}
	FILE *fp = fopen((char *)payload, "wb");
	fwrite(file_payload, 1, (size_t)len_of_payload, fp);
	fclose(fp);
	printf("File downloaded.\n");
    
	return 1;
}

int put_cmd(void *payload)
{
	if (conn != 1) {
		printf("ERROR: You did not open any connection.\n");
		return -1;
	}
	if (auth != 1) {
		printf("ERROR: You were not granted authentication.\n");
		return -1;
	}
    
	// send PUT_REQUEST
	FILE *fp;
	if ((fp = fopen((char *)payload, "rb")) == NULL) {
		printf("ERROR: The file is not existed.\n");
		return -1;
	}
	struct message_s PUT_REQUEST;
	memcpy(PUT_REQUEST.protocol, myftp_protocol, 6);
	PUT_REQUEST.type = 0xA9;
    
	// PUT_REQUEST.status = unused;
	PUT_REQUEST.length = htonl(12 + strlen(payload) + 1);
	send_packet(&PUT_REQUEST, 12);
	send_packet(payload, strlen(payload) + 1);
    
	// wait and receive GET_REPLY
	struct message_s PUT_REPLY;
	receive_packet(&PUT_REPLY, 12);
	if (memcmp(PUT_REPLY.protocol, myftp_protocol, 6) != 0 || PUT_REPLY.type != (char)0xAA || ntohl(PUT_REPLY.length) != 12) {
		printf("ERROR: Received wrong data. Connection closed.\n");
		close(sd);
		conn = 0;
		return -1;
	}
    
	// send FILE_DATA
	struct message_s FILE_DATA;
	memcpy(FILE_DATA.protocol, myftp_protocol, 6);
	FILE_DATA.type = 0xFF;
	fseek(fp, 0, SEEK_END);
	int len_of_payload = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	FILE_DATA.length = htonl(12 + len_of_payload);
	send_packet(&FILE_DATA, 12);
	void *file_payload = malloc(len_of_payload);
	fread(file_payload, (size_t)len_of_payload, 1, fp);
	fclose(fp);
	send_packet(file_payload, len_of_payload);
	printf("File uploaded.\n");
    
	return 1;
}

int quit_cmd()
{
	if (conn != 1 || auth != 1) {
		return -1;
	}
    
	// send QUIT_REQUEST
	struct message_s QUIT_REQUEST;
	memcpy(QUIT_REQUEST.protocol, myftp_protocol, 6);
	QUIT_REQUEST.type = 0xAB;
	QUIT_REQUEST.status = '\0';
	QUIT_REQUEST.length = 12;
	send_packet(&QUIT_REQUEST, QUIT_REQUEST.length);
    
	// wait and receive QUIT_REPLY
	struct message_s QUIT_REPLY;
	receive_packet(&QUIT_REPLY, 12);
	if (memcmp(QUIT_REPLY.protocol, myftp_protocol, 6) != 0 || QUIT_REPLY.type != (char)0xAC || ntohl(QUIT_REPLY.length) != 12) {
		printf("ERROR: Received wrong data. Connection closed.\n");
		close(sd);
		conn = 0;
		return -1;
	}
	conn = close(sd);
	printf("Thank you\n");
	return 0;
}

void exit_program(int sig)
{
	printf("\nProgram have been terminated.\n");
	quit_cmd();
	close(sd);
	exit(0);
}

int main(int argc, char *argv[])
{
	signal(SIGINT, exit_program);
	while (1) {
		char buff[100];
		memset(buff, 0, 100);
		printf("%s", "Client> ");
		scanf("%s", buff);
		if (strcmp(buff, "open") == 0) {
			char *ip;
			int port = 1;
			scanf("%s", buff);
			ip = malloc((size_t)strlen(buff));
			strcpy(ip, buff);
			scanf("%s", buff);
			port = atoi(buff);
			conn = open_cmd(ip, port);
		} else if (strcmp(buff, "auth") == 0) {
			char *payload = malloc(256);
			/* Get the name pass, with size limit */
			scanf(" %256[0-9a-zA-Z ]s", payload);
			auth = auth_cmd(payload);
		} else if (strcmp(buff, "ls") == 0) {
			ls_cmd();
		} else if (strcmp(buff, "get") == 0) {
			char *payload = malloc(256);
			/* Get the name pass, with size limit */
			scanf(" %256[0-9a-zA-Z._-]s", payload);
			get_cmd(payload);
		} else if (strcmp(buff, "put") == 0) {
			char *payload = malloc(256);
			/* Get the name pass, with size limit */
			scanf(" %256[0-9a-zA-Z._-]s", payload);
			put_cmd(payload);
		} else if (strcmp(buff, "quit") == 0) {
			auth = quit_cmd();
			break;
		} else {
			printf("Wrong command.\n");
		}
	}
	return 0;
}
