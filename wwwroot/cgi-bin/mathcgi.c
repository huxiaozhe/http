#include <stdio.h>
#include <stdlib.h>

void mymath(char *arg){
	char * argv[3];
	int i = 0;
	char *start = arg;
	while(*start){
		if(*start == '='){
			start++;
			argv[i++] = start;
			continue;
		}
		if(*start = '&'){
			*start = '\0';
		}
		start++;
	}
	argv[i] = NULL;
	int data1 = atoi(argv[0]);
	int data2 = atoi(argv[1]);
	printf("<html<body><h1>");
	printf("%d + %d = %d<br/>", data1, data2, data1 + data2);
	printf("%d - %d = %d<br/>", data1, data2, data1 - data2);
	printf("%d * %d = %d<br/>", data1, data2, data1 * data2);
	printf("%d / %d = %d<br/>", data1, data2, data2==0?0:data1/data2);
	printf("%d %% %d = %d<br/>", data1, data2, data2==0?0:data1%data2);
	printf("</h1></body></html>");
}

int main(){
	printf("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n");
	char *method = NULL;
	char *query_string = NULL;
	char *string_arg = NULL;
	int content_len = -1;
	char buf[1024] = {0};
	if(method = getenv("METHOD")){
		if(strcasecmp(method, "GET") == 0){
			if((query_string = getenv("QUERY_STRING"))){
				string_arg = query_string;
			}
		}else{
			if(getenv("CONTENT_LENGTH")){
				content_len = atoi(getenv("CONTENT_LENGTH"));
				int i = 0;
				for(; i < content_len; i++){
					read(0, &buf[i], 1);
				}
				buf[i] = '\0';
				string_arg  = buf;
			}
		}
	}
  
	mymath(string_arg);

	return 0;
}
