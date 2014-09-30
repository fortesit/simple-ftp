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
 mkdir filedir
 make all
 ./server_{linux|unix} [PORT]
 
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <ctype.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h>
#include <pthread.h>
#include "myftp.h"

#define DEBUG_MODE 0

pthread_mutex_t mutex;

struct message_s received_item, send_item;
struct sockaddr_in server_addr;
struct threadargs
{
    struct sockaddr_in client_addr;
    int client_socket;
}tas;

const char myftp_protocol[6] = {0xe3,'m','y','f','t','p'};
int server_socket;

void dump_memory(void const* data, size_t len)
{
	if (DEBUG_MODE) {
		size_t i;
		printf("Data in [%p..%p): ", data, data + len);
        for (i = 0; i < len; i++) {
			printf("%02X ", ((unsigned char*)data)[i]);
        }
		printf("\n");
	}
	return;
}

int send_packet(int sd, const void* buffer, int length)
{
	int sentLength = 0;
	while (sentLength < length) {
		int len = (int)send(sd, buffer + sentLength, length - sentLength, 0);
		if (len < 0) {
			printf("ERROR: When sending data, %s (Errno:%d)\n", strerror(errno), errno);
			break;
		}
		sentLength += len;
	}
	if (DEBUG_MODE) {
		printf("Send ");
		dump_memory(buffer,length);
	}
	return sentLength;
}

int receive_packet(int sd, void* buffer, int length)
{
	int receivedLength = 0;
	while (receivedLength < length) {
		int len = (int)recv(sd, buffer + receivedLength, length - receivedLength, 0);
		if (len < 0) {
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

char** readDir(char* path)
{
    char** returnBuffer = (char**)calloc(512, sizeof(char*));
    int return_code;
    DIR *dir;
    struct dirent *entry = (struct dirent*) calloc (sizeof(struct dirent) + 256, 1);
    struct dirent *result ;
    
    if ((dir = opendir(path)) == NULL)
        perror("opendir() error");
    else {
        int index=0;
        while(1){
            return_code = readdir_r(dir, entry, &result);
#ifdef Linux
            if(result == NULL )
#else
                if(return_code==0)
#endif
                    break;
            returnBuffer[index] = (char*) calloc(256, sizeof(char));
            memcpy(returnBuffer[index], entry->d_name, strlen(entry->d_name));
            index++;
        }
        closedir(dir);
    }
    return returnBuffer;
}

void authenticate(int client_socket)
{
    int i, acc_num;
    char line[100], username[100][40], password[100][40], login_username[40], login_password[40], *ptr;
    bool authen_succeeded = false;
    void *payload;
    FILE *fp;
    
    // Wait for AUTH_REQUEST header
    receive_packet(client_socket, &received_item, 12);
    received_item.length = ntohl(received_item.length);
    
    // Wait for AUTH_REQUEST payload
    payload = malloc(received_item.length - 12);
    receive_packet(client_socket, payload, received_item.length - 12);
    ptr = payload;
    ptr = strtok(ptr, " ");
    strcpy(login_username, ptr);
    ptr = strtok(NULL, " ");
    strcpy(login_password, ptr);
    
    // Get username and password from file
    if (!(fp = fopen("access.txt", "r"))) {
        perror("password file error");
        printf("server cannot authenticate...\n");
        pthread_exit(NULL);
    } else {
        strcpy(line, "");
        for (i = 0; fgets(line, 100, fp) != NULL; i++) {
            if (strlen(line) == 1) {
                break;
            }
            ptr = line;
            ptr = strtok(ptr, " ");
            strcpy(username[i], ptr);
            ptr = strtok(NULL, "\n");
            strcpy(password[i], ptr);
        }
        fclose(fp);
        acc_num = i;
        
        // Read AUTH_REQUEST
        if (memcmp(received_item.protocol, myftp_protocol, 6) || (unsigned char)received_item.type != 0xa3) {
            printf("received abnormal data.\n");
            pthread_exit(NULL);
        }
    }
    
    // Check username and password
    for (i = 0; i < acc_num; i++) {
        if (!strcmp(login_username, username[i]) && !strcmp(login_password, password[i])) {
            printf("%s logged in\n", login_username);
            authen_succeeded = true;
        }
    }
    
    // Send AUTH_REPLY
    memcpy(send_item.protocol, myftp_protocol, 6);
    send_item.type = 0xa4;
    send_item.status = authen_succeeded;
    send_item.length = 12;
    send_item.length = htonl(send_item.length);
    send_packet(client_socket, &send_item, 12);
    
    if (!authen_succeeded) {
        printf("Rejected login attempt\n");
        close(client_socket);
        pthread_exit(NULL);
    }
    
    return;
}

void openConnection()
{
    int client_addr_size = sizeof(tas.client_addr);
    pthread_mutex_lock(&mutex);
    
    // Ready for client to connect
    tas.client_socket = accept(server_socket, (struct sockaddr *) &tas.client_addr, (socklen_t *)&client_addr_size);
    printf("Connected from %s:%hu\n", inet_ntoa(tas.client_addr.sin_addr), tas.client_addr.sin_port);
    
    // Wait for OPEN_CONN_REQUEST
    receive_packet(tas.client_socket, &received_item, 12);
    received_item.length = ntohl(received_item.length);
    
    // Read OPEN_CONN_REQUEST
    if (memcmp(received_item.protocol, myftp_protocol, 6) || (unsigned char)received_item.type != 0xa1) {
        printf("received abnormal data.\n");
        close(tas.client_socket);
        exit(1);
    }
    
    // Send OPEN_CONN_REPLY
    memcpy(send_item.protocol, myftp_protocol, 6);
    send_item.type = 0xa2;
    send_item.status = 0x01;
    send_item.length = 12;
    send_item.length = htonl(send_item.length);
    send_packet(tas.client_socket, &send_item, 12);
    printf("Connection opened\n");
    
    return;
}

void acceptClient(int port)
{
    long val = 1;
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(long)) == -1) {
        perror("setsockopt");
        exit(1);
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);
    if (bind(server_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("bind error");
        exit(1);
    }
    if (listen(server_socket, SOMAXCONN) < 0) {
        perror("listen error");
        exit(1);
    }
}

void listFile(int client_socket)
{
    // Read directory
    char** c = readDir("filedir/"), filenames[99999];
    int i = 0;
    strcpy(filenames, "");
    while (c[i] != NULL) {
        if (strcmp(c[i], ".") && strcmp(c[i], "..")) {
            strcat(filenames, strcat(c[i], "\n"));
        }
        i++;
    }
    
    // Send LIST_REPLY
    memcpy(send_item.protocol, myftp_protocol, 6);
    send_item.type = 0xa6;
    send_item.length = 12+(int)strlen(filenames)+1;
    send_item.length = htonl(send_item.length);
    send_packet(client_socket, &send_item, 12);
    send_packet(client_socket, filenames, (int)strlen(filenames)+1);
    printf("Sent LIST_REPLY\n");
    
}

void uploadFile(struct message_s PUT_REQUEST, int client_socket)
{
	printf("receive PUT_REQUEST\n");
    
	// Receive PUT_REQUEST
	if (PUT_REQUEST.length < 13) {
		printf("ERROR: Received wrong data. Command ignored.\n");
		return;
	}
	void *payload = malloc(256);
	receive_packet(client_socket, payload, PUT_REQUEST.length - 12);
	printf("send PUT_REPLY\n");
    
	// Send PUT_REPLY
	struct message_s PUT_REPLY;
	memcpy(PUT_REPLY.protocol, myftp_protocol, 6);
	PUT_REPLY.type = 0xAA;
    
	// PUT_REPLY.status = unused;
	PUT_REPLY.length = htonl(12);
	send_packet(client_socket, &PUT_REPLY, 12);
	printf("wait and receive FILE_DATA\n");
    
	// Wait and receive FILE_DATA
	struct message_s FILE_DATA;
	receive_packet(client_socket, &FILE_DATA, 12);
	int len_of_payload = ntohl(FILE_DATA.length) - 12;
	void *file_payload = malloc(len_of_payload);
	receive_packet(client_socket, file_payload, len_of_payload);
	if (memcmp(FILE_DATA.protocol, myftp_protocol,6) !=0 || FILE_DATA.type != (char)0xFF || ntohl(FILE_DATA.length) < 12) {
		printf("ERROR: Received wrong data. Connection closed.\n");
		return;
	}
	char *filename = malloc(280);
	strcpy(filename, "./filedir/");
	strcat(filename, (char*)payload);
	FILE *fp = fopen(filename, "wb");
	fwrite(file_payload, 1, (size_t)len_of_payload, fp);
	fclose(fp);
	printf("File uploaded.\n");
    
	return;
}

void downloadFile(struct message_s GET_REQUEST, int client_socket)
{
	// Receive GET_REQUEST
	if (GET_REQUEST.length < 13) {
		printf("ERROR: Received wrong data. Command ignored.\n");
		return;
	}
	void *payload = malloc(256);
	receive_packet(client_socket, payload, GET_REQUEST.length - 12);
    
	// Send GET_REPLY
	struct message_s GET_REPLY;
	memcpy(GET_REPLY.protocol, myftp_protocol, 6);
	GET_REPLY.type = 0xA8;
	FILE *fp;
	char *filename = malloc(280);
	strcpy(filename, "./filedir/");
	strcat(filename, (char*)payload);
	if ((fp = fopen(filename, "rb")) == NULL) {
		printf("ERROR: The request file is not existed.\n");
		GET_REPLY.status = 0;
	} else {
		GET_REPLY.status = 1;
	}
	GET_REPLY.length = htonl(12);
	send_packet(client_socket, &GET_REPLY, 12);
	
	if (GET_REPLY.status == 0) {
		return;
	}
	
	//send FILE_DATA
	struct message_s FILE_DATA;
	memcpy(FILE_DATA.protocol, myftp_protocol, 6);
	FILE_DATA.type = 0xFF;
	fseek(fp, 0, SEEK_END);
	int len_of_payload = (int)ftell(fp);
	fseek(fp, 0, SEEK_SET);
	FILE_DATA.length = htonl(12 + len_of_payload);
	send_packet(client_socket, &FILE_DATA, 12);
	void *file_payload = malloc(len_of_payload);
	fread(file_payload, (size_t)len_of_payload, 1, fp);
	fclose(fp);
	send_packet(client_socket, file_payload, len_of_payload);
	printf("File downloaded.\n");
    
	return;
}

void quit(int client_socket, struct sockaddr_in client_addr)
{
    // Send QUIT_REPLY
    memcpy(send_item.protocol, myftp_protocol, 6);
    send_item.type = 0xac;
    send_item.length = 12;
    send_item.length = htonl(send_item.length);
    send_packet(client_socket, &send_item, 12);
    printf("Connection from %s:%hu is closed\n", inet_ntoa(client_addr.sin_addr), client_addr.sin_port);
}

bool waitForOperation(int client_socket, struct sockaddr_in client_addr)
{
    // Wait for request
    receive_packet(client_socket, &received_item, 12);
    received_item.length = ntohl(received_item.length);
    
    // Read and determine type of request
    if (memcmp(received_item.protocol, myftp_protocol, 6) != 0) {
        printf("received abnormal data.\n");
        return false;
    }
    switch ((unsigned char)received_item.type) {
        case 0xa5:
            listFile(client_socket);
            return false;
        case 0xa7:
            downloadFile(received_item, client_socket);
            return false;
        case 0xa9:
            uploadFile(received_item, client_socket);
            return false;
        case 0xab:
            quit(client_socket, client_addr);
            break;
        default:
            printf("received abnormal data.\n");
            return false;
    }
    
    return true;
}

void * pthread_prog(void * args)
{
    struct threadargs foo = *(struct threadargs*)args;
    authenticate(foo.client_socket);
    while (!waitForOperation(foo.client_socket, foo.client_addr));
	return 0;
}

int main(int argc, char *argv[])
{
    pthread_t thread;
    pthread_mutex_init(&mutex, NULL);
    
    if (!argv[1]) {
        printf("Usage: %s [port]\n", argv[0]);
        exit(1);
    }
    acceptClient(atoi(argv[1]));
    
    while (1) {
        openConnection();
        pthread_mutex_unlock(&mutex);
        
        // Create thread
        pthread_create(&thread, NULL, pthread_prog, &tas);
    }
    close(tas.client_socket);
    close(server_socket);
	return 0;
}
