#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<string.h>
#include<ctype.h>
#include<arpa/inet.h>
#include<errno.h>
#include<sys/stat.h>
#include<pthread.h>
 
#define SERVER_PORT  80     //服务器的端口号
 
static int Debug = 1;
 
//处理http请求
void* do_http_request(void* client_sock);
 
//返回 -1 表示读取失败   0 表示读取到一个空行  >0 表示读取成功
int get_line(int sock ,char *buf ,int size );
 
//回复客户端请求
void do_http_response1( int client_sock );
 
void do_http_response( int client_sock , const char *path );
 
//响应404
void not_found( int client_sock );
 
//发送 http 头部 
int header(int client_sock ,FILE *fp);
	
//发送http body 
void cat(int client_sock , FILE *fp);
 
//响应501  客户端的请求方式异常
void unimplemented(int client_sock );
 
//响应400 客户端发送的请求格式有问题
void bad_request( int client_sock );
 
//500
void Internal_Error(int client_sock);
 
 
int main(void){
    
    int sock;//代表信箱
    struct sockaddr_in server_addr; //标签,保存端口号,ip地址等
 
    //1, 创建信箱
    sock = socket( AF_INET , SOCK_STREAM , 0);
    
    //2. 清空标签,写上地址和端口号
    bzero( &server_addr ,sizeof(server_addr));
    server_addr.sin_family = AF_INET; //选择协议族IPV4
    server_addr.sin_addr.s_addr = htonl( INADDR_ANY ); //监听本地所有ip地址
    server_addr.sin_port = htons( SERVER_PORT ); //绑定我们的端口号
 
    //3. 将我们的标签贴到信箱上
    bind(sock ,(struct sockaddr *)&server_addr,sizeof(server_addr));
 
    //4. 将我们的信箱挂到传达室,这样,保安就可以接收信件了
    listen(sock, 128);    //这里的128表示同时可以接收的信件数量
 
    //万事俱备,只等美女来信
    printf("等待美女的来信\n");
   
    int done =1;
    //不断接受来信
    while( done ){
    	struct sockaddr_in client;
		int client_sock,len;
		char client_ip[64];
		char buff[256];
		pthread_t id ;   //线程的句柄 
		int *pclient_sock=NULL;
 
		socklen_t client_addr_len;
		client_addr_len = sizeof(client);
		client_sock = accept(sock ,(struct sockaddr *)&client, &client_addr_len);
        //打印客户端ip地址和端口号
		printf("client ip: %s\t port : %d\n",
	      inet_ntop( AF_INET, &client.sin_addr.s_addr,client_ip,
		  sizeof(client_ip)), ntohs(client.sin_port));
	
		//5 、处理http请求 ，读取客户端发送的数据   read()
		//do_http_request(client_sock);
		// 5.1 启动线程实现并发
		pclient_sock = (int *)malloc( sizeof(int));
		*pclient_sock = client_sock;
		pthread_create( &id ,NULL , do_http_request ,(void *)pclient_sock );
		
		//6 . 响应客户端请求
		
		//close(client_sock);
		
    }
    close(sock);
    return 0;
}
 
void* do_http_request(void* pclient_sock){
	//读取http请求
	
	//按行读取请求头部
	int len = 0;
	char buf[256]={0};
	char method[64]={0}; //存放请求方法
	char url[256]={0};//存放url(请求的资源)
	char version[64]={0};//协议版本
	char path[256]={0};//url对应的路径
	int client_sock = *(int *)pclient_sock ; 
	
	len = get_line(client_sock , buf ,sizeof(buf) );
	
	if( len > 0 ){//读取到请求行
		int i=0 , j=0;
		while(!isspace(buf[j]) && i < sizeof(method)-1 ){
			method[i] = buf[j];
			i++;
			j++;
		}
		
		method[i] = '\0';
		if(Debug)   printf("request method = %s\n",method );
		//判断是否GET方法,这个服务器我们只处理get请求
	    if( strncasecmp( method ,"GET",i)==0 ){
			if(Debug)   printf("method GET\n");
			
			//继续读取请求资源
			while(isspace(buf[j++]));//跳过空格
			i = 0;
			while(!isspace(buf[j]) && i < sizeof(url)-1){
				url[i] = buf[j];
				i++;
				j++;
			}
			//手动加上字符串结束符
			url[i] = '\0';
			if(Debug)  printf("request url = %s\n",url);
			//printf("131 : %c\n",buf[j]);
			//读取协议版本
			
			while(isspace(buf[j++]));//跳过空格
			
			i = 0;
			while(!isspace(buf[j]) && i < sizeof(version)-1){
				version[i] = buf[j];
				i++;
				j++;
			}
			//手动加上字符串结束符
			version[i] = '\0';
			if(Debug)  printf("request version = %s\n",version);
			
			//打印请求头部
			do{
		       len = get_line(client_sock , buf ,sizeof(buf) );
		       printf("request head = %s\n" , buf );
	        }while(len > 0);
			
			//****定位到服务器本地的  html文件 ****
			//****如果需要修改资源 , 那么将这里的path 路径改成你自己资源对应的路径****
			
			//处理 url 中的?
			{
				char *pos = strchr( url ,'?');
				if(pos){
					*pos = '\0';
					printf("real url= %s\n",url);
				}
				sprintf(path ,"./html_file/%s", url );
				if(Debug)  printf("path = %s\n", path );
				
				//判断请求的资源是否存在 ,存在则进行响应
				struct stat st;      //保存文件信息的结构体
				if( stat( path , &st) == -1 ){//如果不存在则响应  404  NOT FOUND
				    fprintf( stderr ," stat :%s failed . reason :%s\n", path , strerror(errno));
					not_found( client_sock );
				}else{ //执行http响应
					
					if(S_ISDIR(st.st_mode)){ //如果是一个目录,则补全
						strcat(path , "/indext.html");
					}
					//响应给客户端
					do_http_response(client_sock , path );
				}
				
			}
			
		}else{ //客户端的请求方式不是 get  , 响应 501 Method Not Implemented
			fprintf(stderr ,"warning  other method[%s]\n",method);
			do{
		       len = get_line(client_sock , buf ,sizeof(buf) );
		       printf("request head = %s\n" , buf );
	        }while(len > 0);
			//响应客户端 501 Method Not Implemented
			unimplemented(client_sock);
		}	
	}else {   //客户端发送的请求格式有问题 相应 400  BAD REQUEST
		bad_request( client_sock);
	}
	close(client_sock);
	if( pclient_sock )free(pclient_sock);
	return NULL;
}
 
void do_http_response( int client_sock , const char *path ){
	FILE  *fp = NULL;
	fp = fopen( path ,"rb");
	if( fp==NULL ){
		not_found(client_sock);
		return ;
	}
	//文件打开成功
	
	//1. 发送 http 头部 
	int ret = header(client_sock , fp);
	
	//2. 发送http body
	if( !ret ){	
		cat(client_sock , fp);
	}
	//3. 清理资源
	fclose( fp );
}
 
int header(int client_sock ,FILE *fp){
	char buf[1024] = { 0 };
	char temp[64];
	int fpId = -1;
	struct stat st;
	
	strcat(buf , "HTTP/1.1 200 OK\r\n");
	strcat(buf , "Server:wServer\r\n");
	strcat(buf , "Content-Type:text/html\r\n");
	strcat(buf , "Connection:Close\r\n");
	
	fpId = fileno( fp );
	if( fstat( fpId ,&st) == -1  ){
		//响应客户端 500 Internal Server Error
		Internal_Error(client_sock); 
		return -1;
	}
	//将响应资源的大小加上
	snprintf( temp , 64 ,"Content-Length:%ld\r\n\r\n" , st.st_size );
	strcat( buf , temp);
	
	if(Debug) fprintf(stdout ," header :%s\n", buf );
	
	if( send( client_sock , buf , strlen(buf) , 0 ) < 0 ){
		fprintf( stderr ," send failed . reason: %s\n", strerror(errno));
		return -1;
	}
	return 0;
}
	
void cat(int client_sock , FILE *fp){
	char buf[1024] = { 0 };
	
	// 1 .没次读取一个字符
	/*
	while( fread(buf , 1 , sizeof(buf), fp) >0 ){
		int len = write( client_sock , buf , strlen(buf));
		
		memset( buf , 0 ,sizeof(buf));
	}*/
	
	fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
 
    // 为文件分配缓冲区
    char* buffer =  (char*)malloc(size);
 
    // 读取文件数据
    size_t bytesRead = fread(buffer, 1, size, fp);
    if (bytesRead != size) {
        printf("读取文件时发生错误\n");
       return ;
    }
	
    // 使用文件数据进行操作...
     printf("读取 %lu 字节的数据\n", bytesRead);
	 int len = write( client_sock , buffer , size);
	 if( len < 0){
		 fprintf(stderr , "write failed . reason :%s\n", strerror(errno));
	 }
   
    free(buffer);
	
	// 2. 没次发
	
	//-- 无图片的版本
	/*fgets( buf ,sizeof(buf) , fp);
	while( !feof(fp)){
		int len = write( client_sock , buf , strlen(buf));
		if(len < 0){
			fprintf(stderr , "write failed . reason :%s\n", strerror(errno));
			break;
		}
		fprintf(stdout , "%s", buf);
		fgets( buf ,sizeof(buf) , fp);
	}*/
}
 
void do_http_response1( int client_sock){
	//main_header   ->  状态行 + 消息报头
    //响应代号 200 服务器存在请求资源,可以响应给客户端
	const char *main_header = "\
HTTP/1.1 200 OK\r\n\
Server:wServer\r\n\
Content-Type:text/html\r\n\
Connection:Close\r\n";
	//回响正文
	const char* welcome_content="\
<html lang = \"zh-CN\">\n\
<head>\n\
<meta content = \"text/html;charset=utf-8\"http-equiv=\"Content-Type\">\n\
<title>Tihis is a test</title>\n\
</head>\n\
<body>\n\
<dir align = center height =\"500px\">\n\
<br/><br/><br/>\n\
<h2>早上好</h2><br/><br/>\n\
<form action = \"commit\"method = \"post\">\n\
尊姓大名:<input type =\"text\" name=\"name\"/>\n\
<br/> 芳龄几何:<input type =\"password\" name =\"age\"/>\n\
<br/><br/><br/><input type =\"submit\" value=\"提交\"/>\n\
<input type =\"reset\" value =\"重置\"/>\n\
</form>\n\
</dir>\n\
</body>\n\
</html>";
	
	if( Debug ) fprintf(stdout , ".....do http_response.....\n");
	
	// 1 .发送main_header
	int len = write( client_sock , main_header ,strlen(main_header));
	if(Debug) fprintf(stdout , "main_header[%d] : %s\n", len  ,main_header);
	
	// 2 .生成 Conten-Lenght(要发送的文件大小) ,并发送
	int wc_len = strlen( welcome_content );
	char sent_buf[64]={0};
	len = snprintf( sent_buf , 64 ,"Content-Length:%d\r\n\r\n" , wc_len );
	len = write(client_sock , sent_buf , len );
	if(Debug)  fprintf(stdout , "sent_buf[%d] : %s" , len ,sent_buf );
	
	// 3. 发送html文件
	len = write( client_sock , welcome_content , wc_len);
	if( Debug ) fprintf(stdout, "write[%d] : %s" , len , welcome_content );
								  
}
 
int get_line(int sock , char *buf ,int size ){
	//每次读取一个字符
	int count = 0 ;   //当前已经读取到的字符大小
    char ch = '\0';  
	int len = 0 ;
	while( (count < size -1) && ch!='\n'){
		len  = read(sock , &ch ,1);
		if( len == 1 ){  //正常读取成功
		    //处理 '\r（回车）  \n'
			if( ch=='\r')   continue;
			else if(ch=='\n'){
				//buf[count] = '\0';     //手动添加字符串结束符
				break;
			}
		//处理一般的字符
		buf[count++] = ch;
		}else if( len < 0 ){   //读取出错的情况
			perror("read fialed\n");
			count = -1;
			break;
		}else {   //len = 0 的情况   客户端关闭了sock连接
		    fprintf(stderr , "client close\n");
			count = -1 ;
			break;
		}
	}
	if( count >=0)  buf[count]='\0';
	//if(Debug)  printf("line information %s\n", buf);
	return count;  
}
 
void not_found( client_sock ){
	//状态行 + 消息报头
	const char *reply = "\
HTTP/1.1 404 NOT FOUND\r\n\
Server:wServer\r\n\
Content-Type:text/html\r\n\
Connection:Close\r\n";
 
	//404
	const char *sent_buf = "\
<!DOCTYPE html>\n\
<html lang=\"zh-CN\">\n\
<head>\n\
<meta charset=\"UTF-8\">\n\
<title>404 页面未找到</title>\n\
<style>\n\
body {\n\
text-align: center;\n\
padding: 150px;\n\
font-family: \"Helvetica Neue\", Helvetica, Arial, sans-serif;\n\
}\n\
h1 {\n\
font-size: 50px;\n\
}\n\
body {\n\
font-size: 20px;\n\
}\n\
article {\n\
display: block;\n\
text-align: left;\n\
width: 650px;\n\
margin: 0 auto;\n\
}\n\
a {\n\
color: #00a2e8;\n\
text-decoration: none;\n\
}\n\
a:hover {\n\
text-decoration: underline;\n\
}\n\
</style>\n\
</head>\n\
<body>\n\
<article>\n\
<h1>404 NOT FOUND</h1>\n\
<div>\n\
<p>抱歉，您试图访问的页面不存在。可能是链接错误或页面已被移除。</p>\n\
<p>您可以点击下面的链接返回<a href=\"/\">首页</a>或者进行搜索。</p>\n\
</div>\n\
</article>\n\
</body>\n\
</html>";
	
	
     int len = write( client_sock , reply ,strlen(reply));
	 if( Debug )  fprintf(stdout , "reply[%d] : %s",len , reply); 
	 // 发送 Conten-Lenght 
	 int sent_buf_size = strlen( sent_buf);
	 char content_lenght[64] ={0};
	 len = snprintf( content_lenght , 64 ,"Content-Length:%d\r\n\r\n", sent_buf_size );
	printf("1\n");
    //  len = write( client_sock , content_lenght , len );
    printf("2\n");
	 if( Debug ) fprintf(stdout , "content_lenght[%d]:%s", len , content_lenght);
	 
	 // 3. 发送响应正文
	 len = write( client_sock , sent_buf , sent_buf_size );
	 if(Debug) fprintf(stdout ,"%s", sent_buf);
}
 
void unimplemented(int client_sock ){
		//状态行 + 消息报头
	const char *reply = "\
HTTP/1.1 501 Method Not Implemented\r\n\
Server:wServer\r\n\
Content-Type:text/html\r\n\
Connection:Close\r\n";
 
	//501
	const char *sent_buf = "\
<!DOCTYPE html>\n\
<html>\n\
<head>\n\
<meta charset=\"utf-8\">\n\
<title>HTTP 状态 501 – Method Not Implemented</title>\n\
<style type=\"text/css\">\n\
h1{\n\
font-family: Tahoma,Arial,scans-serif;\n\
color: white;\n\
background-color: #525d76;\n\
font-size: 30px;\n\
}\n\
body{\n\
font-family: Tahoma,Arial,scans-serif;\n\
color: black;\n\
background-color: white;\n\
}\n\
b{\n\
font-family: Tahoma,Arial,scans-serif;\n\
color: white;\n\
background-color: #525d76;\n\
}\n\
p{\n\
font-family:Tahoma,Arial,scans-serif;\n\
background: white;\n\
color: black;\n\
}\n\
h3{\n\
font-family: Tahoma,Arial,scans-serif;\n\
color: white;\n\
background-color: #525d76;\n\
}\n\
</style>\n\
</head>\n\
<body>\n\
<h1 >HTTP 状态 501 – Method Not Implemented</h1>\n\
<hr class=\"line\">\n\
<p ><b >类型</b>异常报告</p>\n\
<p ><b>消息</b> 执行出异常</p>\n\
<p ><b>描述</b>  客户端请求方法有异常</p>\n\
<hr class=\"line\">\n\
<h3> Linux</h3>\n\
</body>\n\
</html>";
	
     int len = write( client_sock , reply ,strlen(reply));
	 if( Debug )  fprintf(stdout , "reply[%d] : %s",len , reply); 
	 // 发送 Conten-Lenght 
	 int sent_buf_size = strlen( sent_buf);
	 char content_lenght[64] ={0};
	 len = snprintf( content_lenght , 64 ,"Content-Length:%d\r\n\r\n", sent_buf_size );
	 len = write( client_sock , content_lenght , len );
	 if( Debug ) fprintf(stdout , "content_lenght[%d]:%s", len , content_lenght);
	 
	 // 3. 发送响应正文
	 len = write( client_sock , sent_buf , sent_buf_size );
	 if(Debug) fprintf(stdout ,"%s", sent_buf);
}
 
void bad_request( int client_sock ){
	//状态行 + 消息报头
	const char *reply = "\
HTTP/1.1 400 BAD REQUEST\r\n\
Server:wServer\r\n\
Content-Type:text/html\r\n\
Connection:Close\r\n";
 
	//400
	const char *sent_buf = "\
<!DOCTYPE html>\n\
<html>\n\
<head>\n\
<meta charset=\"utf-8\">\n\
<title>HTTP 状态 400 – Method Not Implemented</title>\n\
<style type=\"text/css\">\n\
h1{\n\
font-family: Tahoma,Arial,scans-serif;\n\
color: white;\n\
background-color: #525d76;\n\
font-size: 30px;\n\
}\n\
body{\n\
font-family: Tahoma,Arial,scans-serif;\n\
color: black;\n\
background-color: white;\n\
}\n\
b{\n\
font-family: Tahoma,Arial,scans-serif;\n\
color: white;\n\
background-color: #525d76;\n\
}\n\
p{\n\
font-family:Tahoma,Arial,scans-serif;\n\
background: white;\n\
color: black;\n\
}\n\
h3{\n\
font-family: Tahoma,Arial,scans-serif;\n\
color: white;\n\
background-color: #525d76;\n\
}\n\
</style>\n\
</head>\n\
<body>\n\
<h1 >HTTP 状态 400 – Method Not Implemented</h1>\n\
<hr class=\"line\">\n\
<p ><b >类型</b>异常报告</p>\n\
<p ><b>消息</b> 执行出异常</p>\n\
<p ><b>描述</b>  客户端请求格式异常</p>\n\
<hr class=\"line\">\n\
<h3> Linux</h3>\n\
</body>\n\
</html>";
	
     int len = write( client_sock , reply ,strlen(reply));
	 if( Debug )  fprintf(stdout , "reply[%d] : %s",len , reply); 
	 // 发送 Conten-Lenght 
	 int sent_buf_size = strlen( sent_buf);
	 char content_lenght[64] ={0};
	 len = snprintf( content_lenght , 64 ,"Content-Length:%d\r\n\r\n", sent_buf_size );
	 len = write( client_sock , content_lenght , len );
	 if( Debug ) fprintf(stdout , "content_lenght[%d]:%s", len , content_lenght);
	 
	 // 3. 发送响应正文
	 len = write( client_sock , sent_buf , sent_buf_size );
	 if(Debug) fprintf(stdout ,"%s", sent_buf);
}
 
void Internal_Error(int client_sock){
	//状态行 + 消息报头
	const char *reply = "\
HTTP/1.1 500 Internal Server Error\r\n\
Server:wServer\r\n\
Content-Type:text/html\r\n\
Connection:Close\r\n";
 
	//500
	const char *sent_buf = "\
<!DOCTYPE html>\n\
<html>\n\
<head>\n\
<meta charset=\"utf-8\" />\n\
<title>HTTP 状态 500 – 内部服务器错误</title>\n\
<style type=\"text/css\">\n\
h1{\n\
font-family: Tahoma,Arial,scans-serif;\n\
color: white;\n\
background-color: #525d76;\n\
font-size: 30px;\n\
}\n\
body{\n\
font-family: Tahoma,Arial,scans-serif;\n\
color: black;\n\
background-color: white;\n\
}\n\
b{\n\
font-family: Tahoma,Arial,scans-serif;\n\
color: white;\n\
background-color: #525d76;\n\
}\n\
p{\n\
font-family:Tahoma,Arial,scans-serif;\n\
background: white;\n\
color: black;\n\
}\n\
h3{\n\
font-family: Tahoma,Arial,scans-serif;\n\
color: white;\n\
background-color: #525d76;\n\
}\n\
</style>\n\
</head>\n\
<body>\n\
<h1 >HTTP 状态 500 – Internal Server Error</h1>\n\
<hr class=\"line\">\n\
<p ><b >类型</b>异常报告</p>\n\
<p ><b>消息</b> 执行出异常</p>\n\
<p ><b>描述</b> 服务器收到请求, 因为自身原因没发响应。</p>\n\
<hr class=\"line\">\n\
<h3> Linux</h3>\n\
</body>\n\
</html>";
	
	
     int len = write( client_sock , reply ,strlen(reply));
	 if( Debug )  fprintf(stdout , "reply[%d] : %s",len , reply); 
	 // 发送 Conten-Lenght 
	 int sent_buf_size = strlen( sent_buf);
	 char content_lenght[64] ={0};
	 len = snprintf( content_lenght , 64 ,"Content-Length:%d\r\n\r\n", sent_buf_size );
	 len = write( client_sock , content_lenght , len );
	 if( Debug ) fprintf(stdout , "content_lenght[%d]:%s", len , content_lenght);
	 
	 // 3. 发送响应正文
	 len = write( client_sock , sent_buf , sent_buf_size );
	 if(Debug) fprintf(stdout ,"%s", sent_buf);
}
 
 
 