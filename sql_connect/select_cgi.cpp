#include "sql_api.h"

void selectFromDb(const char *string_arg){
	char arg_buf[1024];
	strcpy(arg_buf, string_arg);
	char *myargv[6];
	char *start = arg_buf;
	int i = 0;
	while(*start){
		if(*start == '='){
			start++;
			myargv[i++] = start;
			continue;
		}
		if(*start == '&'){
			*start = '\0';
		}
		start++;
	}
	myargv[i] = NULL;
	sqlApi mydb;
	mydb.select();	
}

int main(){
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

	selectFromDb(string_arg);
	return 0;
}
