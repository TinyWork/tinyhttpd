/*
 *
 *  tiny web service which can handle simple http GET and POST request;
 *  FREE FOR USE
 *  created by salt 2015
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <pthread.h>


#define BUF_SIZE 1024
#define LINE_BUF 100  // single line

// request processr
void* request_handler(void* arg);

// header info
void header(FILE* clnt,char* file_name);

// content type
char* content_type(char* file_name);

// http get processor
void http_get(FILE* clnt,char* url);

// http post processor
void http_post(FILE* clnt);

// 404 processor
void process_404(FILE* clnt);

// unknow request
void unknown(FILE* clnt); 

// error processor
void error_handler(FILE* clnt);

// server inner error
void error_exit(char* msg);



int main(int argc,char* argv[])
{
    int server_socket,cient_socket;
    struct sockaddr_in server_adr,client_adr;
    socklen_t client_adr_size;
    char buf[BUF_SIZE];
    pthread_t t_id;
    
    if(argc != 2 || atoi(argv[1])<=0 ){
	error_exit("Usage: Set  <port>");
    }

    server_socket = socket(PF_INET, SOCK_STREAM, 0);
    memset(&server_adr, 0, sizeof(server_adr));
    server_adr.sin_family = AF_INET;
    server_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_adr.sin_port=htons(atoi(argv[1]));

    if(bind(server_socket, (struct sockaddr*)&server_adr, sizeof(server_adr)) == -1)
	error_exit("bind() error");

    if(listen(server_socket, 20) == -1)
	error_exit("listen() error");

    while(1){
	client_adr_size = sizeof(client_adr);
	cient_socket = accept(server_socket,(struct sockaddr*)&client_adr, &client_adr_size);
     	printf("New Connection Request: %s:%d",inet_ntoa(client_adr.sin_addr),ntohs(client_adr.sin_port));  	
	pthread_create(&t_id,NULL,request_handler,&cient_socket);
     	pthread_detach(t_id);
    }
 }

void* request_handler(void* arg){
    int client_sock = *((int*)arg);
    char req_line[LINE_BUF];
    char method[255];
    char path[255];
    char url[255];

    // differentiate read and write
    FILE* client_read;
    FILE* client_write;

    client_read = fdopen(client_sock,"r");
    client_write = fdopen(client_sock,"w");

    // first line for http
    fgets(req_line, sizeof(req_line), client_read);
    if(strstr(req_line, "HTTP/") == NULL){
	error_handler(client_write);
 	fclose(client_read);
	fclose(client_write);
	return NULL;
    }    
    
    strcpy(method,strtok(req_line," /"));
    strcpy(url,strtok(NULL, " /"));    
    struct stat st;
    sprintf(path,"webapps/%s",url);
    if(path[strlen(path)-1] == '/')
	strcat(path,"/index.html");
    if(stat(path,&st) == -1){
	process_404(client_write);
	return NULL;
    }    
    if ((st.st_mode & S_IFMT) == S_IFDIR)
            strcat(path, "/index.html"); 

    if(stat(path,&st) == -1){
        process_404(client_write);
        return NULL;
    }
    if(strcmp(method, "GET") == 0){ // GET handler
	http_get(client_write,url);
    }else if(strcmp(method, "POST") == 0){ // POST handler
		
    }else{
	unknown(client_write);
    }
    fclose(client_read);
    close(client_sock);
    return NULL;
}

void http_get(FILE* client_write,char* path){
    char buf[BUF_SIZE];
    FILE* send_file;
    send_file = fopen(path,"r");
    if(send_file == NULL){
	error_handler(client_write);
   	return;
    }
	
    header(client_write,path);

    printf("send begin");
    fgets(buf,sizeof(buf),send_file);
    while(!feof(send_file)){
	fputs(buf,client_write);
	fgets(buf,sizeof(buf),send_file);
    }
    printf("send end\n");
    fflush(client_write);
    fclose(client_write);
    fclose(send_file);
}

void header(FILE* client_write,char* file_name){
    char protocol[] = "HTTP/1.0 200 OK\r\n"; 
    char server[]= "Tiny Web Server \r\n";
    char con_type[BUF_SIZE];
    char ct[BUF_SIZE];
    strcpy(ct,content_type(file_name));
    sprintf(con_type,"Content-type:%s\r\n\r\n","text/html");
    fputs(protocol,client_write);
    fputs(server,client_write);
    fputs(con_type,client_write);
}

char* content_type(char* file){
    char extension[BUF_SIZE];
    char file_name[BUF_SIZE];
    strcpy(file_name,file);
    strtok(file_name,".");
    strcpy(extension,strtok(NULL,"."));
    if(!strcmp(extension,"html") || !strcmp(extension,"htm"))
        return "text/html";
    else
        return "text/plain";
}

void process_404(FILE* client_write){
    char* path_404 = "webapps/404.html";
    struct stat st;
    char buf[BUF_SIZE];
    if(stat(path_404,&st) == -1){
	fputs("404 Page!",client_write);
	fflush(client_write);
	fclose(client_write);
    	return;
    }
    FILE* file_404 = fopen(path_404,"r");
    fgets(buf,sizeof(buf),file_404);
    while(!feof(file_404)){
	fputs(buf,client_write);
  	fgets(buf,sizeof(buf),file_404);
    }  
    printf("404-3\n");
    fflush(client_write);
    fclose(client_write);
    fclose(file_404);
}

void http_post(FILE* client_write){

}

void unknown(FILE* client_write){
    fputs("Unknown Request!",client_write);
    fflush(client_write);
    fclose(client_write);
}

void error_exit(char* msg){
    fputs(msg,stderr);
    fputc('\n',stderr);
    exit(1);
}

void error_handler(FILE* client_write){
    fputs("sth. error happened",client_write);
    fputc('\n',client_write);
}
