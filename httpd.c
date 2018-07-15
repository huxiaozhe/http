#include "httpd.h"

void print_log(char* msg,int level){
#ifndef _STDOUT_
	const char * level_msg[] = {
		"SUCCESS",
		"NOTICE",
		"WARNING",
		"ERROR",
		"FATAL",
	};
	printf("[%s][%s]\n}", msg, level_msg[level%5]);
#endif
}
int startup(const char*ip, int port){
	// 创建套接字函数
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if( sock < 0){
		print_log(strerror(errno), FATAL);
		exit(2);
	}
	int opt = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	// 复用
	struct sockaddr_in local;
	local.sin_family = AF_INET;
	local.sin_port = htons(port);
	local.sin_addr.s_addr = inet_addr(ip);
	if(bind(sock, (struct sockaddr*)&local, sizeof(local)) < 0){
		// 绑定
		print_log(strerror(errno), FATAL);
		exit(3);
	}
	if(listen(sock, 10) < 0){
		print_log(strerror(errno), FATAL);
		exit(4);
	}
	return sock;
}

//ret > 0 line != '\0' , ret = 1 & line='\n ', ret <= 0 && line='\0'
static int get_line(int sock, char line[], int size){
	//一次读取一个字符
	//将所有的 \r \r\n \n 转换为 \n
	char c = '\0';
	int num = 0;
	int len = 0;
	while(c != '\n' && len < size - 1){
		int r = recv(sock, &c, 1, 0);
		if(r > 0){
			if(c == '\r'){
				// 窥探
				int ret = recv(sock , &c, 1, MSG_PEEK);
				if(ret > 0){
					if(c == '\n'){
						recv(sock, &c, 1, 0);
					}else{
						c = '\n';
					}
				}
			}
			// c == \n
			line[len++] = c;
		}else{
			c = '\n';
		}
	}	
	line[len] = '\0';
	return len;
}

static void echo_string(int sock){

}

int echo_www(int sock, char* path, int size){
	//sendfile 函数
	//在内核实现文件描述符之间的拷贝，不需要定义用户空间
	int fd = open(path, O_RDONLY);
	if(fd < 0){
		echo_string(sock);
		print_log(strerror(errno), FATAL);
		return 8;
	}
	const char *echo_line = "HTTP/1.0 200 OK\r\n";
	const char* null_line = "\r\n";
	send(sock, echo_line, strlen(echo_line), 0);
	send(sock, null_line, strlen(null_line), 0);
	if(sendfile(sock, fd, NULL, size) < 0){
		echo_string(sock);
		print_log(strerror(errno), FATAL);
		return 9;
	}
	close(fd);
	return 0;
}

void drop_header(int sock){
	// 丢弃请求报头
	int ret;
	char line[1024];
	do{
		ret = get_line(sock, line,  sizeof(line));
	}while(ret > 0 && strcmp(line, "\n"));
}

int exe_cgi(int sock,char* method,char* path,char* query_string){
	// CGI处理函数
	int content_len = -1;	
	char method_env[SIZE/10];
	char query_string_env[SIZE];
	char content_len_env[SIZE];

	if(strcasecmp(method, "GET") == 0){
		drop_header(sock);
	}else{ //POST
		//消息正文字段大小 Content-Length: 22(CRLF))
		int ret = -1;
		char line[1024];
		do{
			ret = get_line(sock, line,  sizeof(line));
			if(ret > 0 && strncasecmp(line ,"Content-Length: ", 16)){
				//找到消息正文大小	
				content_len = atoi(&line[16]);
			}
		}while(ret > 0 && strcmp(line, "\n"));
		if(content_len == -1){
			echo_string(sock);
			return 10;
		}
	}
	// OK
	const char*echo_line ="HTTP/1.0 200 OK\r\n";
	send(sock, echo_line, strlen(echo_line), 0);
	const char*null_line = "\r\n";
	send(sock, null_line, strlen(null_line), 0);

	int input[2];
	int output[2];
	if(pipe(input) < 0 || pipe(output) < 0){
		echo_string(sock);
		return 11;
	}
	//	父进程作为中间转发
	pid_t pid = fork();
	if(pid < 0){
		echo_string(sock);
		return 12;
	}else if(pid == 0){
		close(input[1]);
		close(output[0]);
		sprintf(method_env, "METHOD=%s", method);
		putenv(method_env);
		// OK
		if(strcasecmp(method, "GET") == 0){
			sprintf(query_string_env,"QUERY_STRING=%s", query_string);	
			putenv(query_string_env);
		}else{ // POST
			sprintf(content_len_env, "CONTENT_LENGTH=%d", content_len);	
			putenv(content_len_env);
		}
		// OK
		//printf("path = %s\n", path);
		dup2(input[0], 0);
		dup2(output[1], 1);
		execl(path, path, NULL);
		exit(1);
	}else{
		close(input[0]);
		close(output[1]);
		int i = 0;
		char c = '\0';
		if(strcasecmp(method, "POST") == 0){
			for(; i<content_len; i++){
				recv(sock, &c, 1, 0);
				write(input[1], &c, 1);
			}
		}
		c  = '\0';
		while(1){
			ssize_t s = read(output[0], &c, 1);
			if(s > 0)
				send(sock, &c, 1, 0);
			else
				break;
		}
		waitpid(pid, NULL,  0);
		close(input[1]);
		close(output[0]);
		close(sock);
	}

}
void* handler_request(void* arg){
	// 处理请求
	// 先接收请求，后返回相应
	int sock = (int)arg;
#ifdef _DEBUG_
	char line[1024];
	memset(line, 0x00, 1024);
	do{
		int ret = get_line(sock, line, sizeof(line));
		if(ret > 0){
			printf("%s",line);
		}else{
			printf("request done\n");
			break;
		}
	}while(1);
#else
	int ret = 0;
	char buf[SIZE];
	char method[SIZE/10];// 方法
	char url[SIZE]; // url 
	char path[SIZE]; // 资源路径
	int i, j;
	int cgi = 0;
	// 工作模式
	if(get_line(sock, buf, sizeof(buf)) <= 0){
		echo_string(sock);
		ret = 5;
		goto end;
	}
	i = 0; // method index
	j = 0; // buf index 
	// GET / /http/1.0
	while(!isspace(buf[j]) && i < sizeof(method) - 1 && j <  sizeof(buf) - 1){
		method[i] = buf[j];
		i++, j++;
	}
	method[i] = '\0';
	if(strcasecmp(method, "GET") && strcasecmp(method, "POST")){
		// 忽略大小写
		echo_string(sock);
		ret = 6;
		goto end;
	}
	if(strcasecmp(method, "POST") == 0){
		// 如果是 POST 方法
		cgi = 1;
	}
	//方法符合预期
	// buf -- GET  /  HTTP/1.0

	// 去除多余的空格
	while(isspace(buf[j]) && j < sizeof(buf)){
		j++;
	}
	i = 0;
	while(!isspace(buf[j])&& j < sizeof(buf) && i < sizeof(url) - 1 ){
		url[i] = buf[j];
		i++, j++;
	}
	url[i] = '\0';
	printf("method url %s %s\n", method, url);
	char *query_string;
	query_string = url;
	while(*query_string != '\0'){
		if(*query_string == '?'){ // 找到了分隔符
			*query_string = '\0';
			query_string++;
			cgi = 1;
			break;
		}
		query_string++;
	}
	sprintf(path, "wwwroot%s", url);
	// method, url, query_string, cgi
	if(path[strlen(path) - 1] == '/'){
		// 如果此时的 url 最后一个字符是 '/'  
		// 则请求的是首页信息
		strcat(path,"index.html");
		printf("path = %s\n", path);
	}
	struct stat st;
	if(stat(path, &st) != 0){
		echo_string(sock);
		ret = 7;
		goto end;
	}else{
		//  OK
		if(S_ISDIR(st.st_mode)){
			printf("2\n");
			// 如果是目录文件
			strcat(path, "/index.html");
		}else if((st.st_mode & S_IXUSR) || \
				(st.st_mode & S_IXGRP) || \
				(st.st_mode & S_IXOTH)){
			cgi = 1;
		}
		if(cgi){
			printf("enter cgi\n");
			// post get 处理 CGI
			exe_cgi(sock, method, path, query_string);
		}
		else{
			// 纯 GET 方法，直接返回资源
			printf("method: %s. url: %s, path: %s, cgi: %d, query_string: %s\n", method, url ,path, cgi, query_string);
			drop_header(sock);
			echo_www(sock, path, st.st_size);
			close(sock);
		}
	}
end:
	printf("quit client...\n");
	close(sock);
	return (void*)ret;
#endif
}









