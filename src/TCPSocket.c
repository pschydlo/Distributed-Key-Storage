#include "TCPSocket.h"

int TCPSocketAccept(int fd){
	
	struct sockaddr_in addr;
	int addrlen = sizeof(struct sockaddr_in);
	
	return accept(fd,(struct sockaddr*)&addr,&addrlen);
}

int TCPSocketListen(int fd){
	
	return listen(fd, 5);

}

int TCPSocketBind(int fd, int port){
	
	int n;
	struct sockaddr_in addr;

	memset((void*)&addr,(int)'\0',sizeof(addr));
	
	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr = htons(INADDR_ANY);
	addr.sin_port        = htons(port);

	if((n=bind(fd,(struct sockaddr*)&addr,sizeof(addr)))==-1)exit(1);
	
	return n;
}

int TCPSocketCreate(){
	int fd;
	
	if((fd=socket(AF_INET,SOCK_STREAM,0))==-1) exit(1); /*ERROR HANDLING TO BE DONE PL0X*/
	
	return fd;
}
	
int TCPSocketConnect(int fd, char * ip, int port){
	
	struct sockaddr_in addr;

	memset((void*)&addr,(int)'\0',sizeof(addr));
	addr.sin_family=AF_INET;
	addr.sin_port=htons(port);
	inet_pton(AF_INET, ip, &(addr.sin_addr));

	return connect(fd,(struct sockaddr*)&addr,sizeof(addr));
}



