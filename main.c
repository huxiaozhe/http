#include <stdio.h>
#include "httpd.h"

static usage(char* proc){
	printf("Usage: %s [local_ip][local_prot]\n", proc);
}

int main(int argc, char* argv[]){
	if(argc != 3){
		usage(argv[0]);
		return 1;
	}
	// 获取监听套接字

	int listen_sock = startup(argv[1], atoi(argv[2]));
	while(1){
		struct sockaddr_in client;
		socklen_t len = sizeof(client);
		int new_sock = accept(listen_sock, (struct sockaddr*)&client, &len);
		// accept
		if(new_sock < 0){
			print_log(strerror(errno),NOTICE);
			continue;
		}

		printf("get client [%s:%d]\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));

		pthread_t id;
		// 创建线程
		int ret = pthread_create(&id, NULL, handler_request, (void*)new_sock);
		if(ret != 0){
			print_log(strerror(errno), WARNING);
			close(new_sock);
			continue;
		}else{
			// 创建成功
			// 防止主线程阻塞 --- 分离该线程
			pthread_detach(id);
		}
	}
	close(listen_sock);
	return 0;
}
