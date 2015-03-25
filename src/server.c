#include "server.h"

#define max(A,B) ((A)>=(B)?(A):(B)) /*I think we don't even use this, we just do it manually in the while loop*/

/*--------- Start Switching Codes ---------- */

/* UI Manager comand hashes */

#define UI_JOIN   592
#define UI_LEAVE  1253
#define UI_SHOW   657
#define UI_RSP 	  350
#define UI_SEARCH 2654
#define UI_BOOPP  1140
#define UI_BOOPS  1143
#define UI_STREAM 2955
#define UI_CLOSESTREAM 69707
#define UI_HASH   586
#define UI_SEND   692
#define UI_EXIT   622
#define UI_CHECK  1097


/* Ring Manager comand hashes */

#define RING_QRY 441
#define RING_RSP 446
#define RING_CON 476

/*TCP Manager comand hashes */

#define TCP_NEW  485
#define TCP_CON  476
#define TCP_SUCC 777
#define TCP_ID   214

/* --------- End Switching Codes ---------- */

struct Server{
	int isBoot;
	int shutdown;
	UDPManager  * udpmanager;
	TCPManager  * tcpmanager;
	RingManager * ringmanager;
	char ip[16];
	int  TCPport;
};

Server * ServerInit(int argc, char ** argv, char * ip){
	Server * server=(Server*)malloc(sizeof(Server));
	
	server->isBoot		= 0;
	server->shutdown	= 0;
	server->TCPport 	= 9000;
	server->udpmanager	= UDPManagerInit();
	server->tcpmanager	= TCPManagerInit();
	
	ServerProcArg(server, argc, argv);
  
	strcpy(server->ip, ip);
	server->ringmanager	= RingManagerInit(server->ip, server->TCPport);
	
	return server;
}

int ServerStart(Server * server){
	
	int maxfd, counter;
	fd_set rfds;
	int n = 0;
  
	Request *request = RequestCreate();
	RequestReset(request);
	
	TCPManagerStart(server->tcpmanager, server->TCPport);
	UDPManagerCreate(server->udpmanager);
	
	while(!(server->shutdown)){
		
		FD_ZERO(&rfds); maxfd=0;
		
		UIManagerArm(&rfds,&maxfd);
		RingManagerArm(server->ringmanager,&rfds,&maxfd);
		TCPManagerArm(server->tcpmanager, &rfds, &maxfd);
		UDPManagerArm(server->udpmanager, &rfds, &maxfd);
		
		counter = select(maxfd+1,&rfds,(fd_set*)NULL,(fd_set*)NULL,(struct timeval*)NULL);
		
		if(counter<0) exit(1);
		if(counter == 0) continue;

		do{
			n = 0;
			
			RequestReset(request);
			if(RingManagerReq(server->ringmanager, &rfds, request)){
				ServerProcRingReq(server, request);
				n++;
			}

			RequestReset(request);
			if(TCPManagerReq(server->tcpmanager, &rfds, request)){
				ServerProcTCPReq(server, request);
				n++;
			}; 

			RequestReset(request);
			if(UDPManagerReq(server->udpmanager, &rfds, request)){
				ServerProcUDPReq(server, request);
				n++;
			}
			
			RequestReset(request);
			if(UIManagerReq(&rfds, request)){
				ServerProcUIReq(server, request);
				n++;
			}
		} while(n != 0);
	}
  
	RequestDestroy(request);
  
	return 0;
}

int ServerStop(Server * server){
	/*To do: close all active fd's*/
	
	UDPManagerStop(server->udpmanager);
	TCPManagerStop(server->tcpmanager);
	RingManagerStop(server->ringmanager);
	free(server);
 
	return 0;
}

int ServerProcUDPReq(Server * server, Request * request){
	
	if(RequestGetArgCount(request) <= 0) return 0;
	if(strcmp(RequestGetArg(request, 0), "OK") == 0) RingManagerSetRing(server->ringmanager, UDPManagerRing(server->udpmanager), UDPManagerID(server->udpmanager));
	if(strcmp(RequestGetArg(request, 0), "NOK") == 0) printf("Problem with UDP\n");
	if(strcmp(RequestGetArg(request, 0), "EMPTY") == 0) UDPManagerReg(server->udpmanager, server->ip, server->TCPport);
	if(strcmp(RequestGetArg(request, 0), "BRSP") == 0){
		/*Stop freaking out, I left this unmodularized on purpose so I wouldn't put it in the wrong place
		 * We can decide to place this entire section in the correct place eventually,
		 * also, this is pretty much just a test*/
		int destID 		= atoi(RequestGetArg(request, 2));
		char * destIP 	= RequestGetArg(request, 3);
		int destPort 	= atoi(RequestGetArg(request, 4));
		
		int id = UDPManagerID(server->udpmanager);
		
		if(id != destID){
		
		int n, idfd = TCPSocketCreate();
		if((n = TCPSocketConnect(idfd, destIP, destPort)) < 0){
			printf("IP: %s, Port: %d", destIP, destPort);
			printf("Could not connect to boot vertex.");
			exit(1);
		} /*ERROR! checking to be done*/
		
		char msg[50];
		sprintf(msg, "ID %d\n", UDPManagerID(server->udpmanager));
  
		write(idfd, msg, strlen(msg));
	/*Until here*/
        }else printf("ID %d occupied, please choose another\n", destID);
	}
		
	int i;
	for(i = 0; i<RequestGetArgCount(request); i++){
		printf("%s,", RequestGetArg(request, i));
		fflush(stdout);
	}
	printf("\n");
	return 0;
}

/* Process Ring Manager Requests */

int ServerProcRingReq(Server * server, Request * request){	
	int argCount = RequestGetArgCount(request);
	if(argCount <= 0) return 0;

	/* Debug Code */	
	
	printf("Ring wrote: ");
	int i = 0;
  for(i = 0; i<RequestGetArgCount(request); i++){
    printf("%s ", RequestGetArg(request, i));
    fflush(stdout);
	} 
	printf("\n");
	
	char * command = RequestGetArg(request,0);
	int code = hash(command);
	
	switch(code){
		case(RING_RSP):
		{
			if(RequestGetArgCount(request) != 6) break; 

			int originID = atoi(RequestGetArg(request, 1));
			int searchID = atoi(RequestGetArg(request, 2));
			
			int    responsibleID    = atoi(RequestGetArg(request, 3)); 
			char * responsibleIP    = RequestGetArg(request, 4);
			int    responsiblePort  = atoi(RequestGetArg(request, 5));

			if(originID == RingManagerId(server->ringmanager)){
				//Handle response 
        /*Simple printf if the UI asked for it?*/
          if(searchID == TCPManagerSearchID(server->tcpmanager)){
            /*Put SUCC sending into a function eventually*/		char msg[50];
            sprintf(msg, "SUCC %d %s %d\n", responsibleID, responsibleIP, responsiblePort);
            write(idfd, msg, strlen(msg));
          }
			}else{
				RingManagerRsp(server->ringmanager, originID, searchID, responsibleID, responsibleIP, responsiblePort);
			}
			
			break;
		}
		case(RING_CON):
		{
			if(RequestGetArgCount(request) != 4) return 0; 
			
			int id = RingManagerId(server->ringmanager);
			
			int    succiID   = atoi(RequestGetArg(request, 1));
			char * succiIP   = RequestGetArg(request, 2); 
			int    succiPort = atoi(RequestGetArg(request, 3)); 

			RingManagerConnect(server->ringmanager, 1, id, succiID, succiIP, succiPort);
			
			break;
		}
		case(RING_QRY):
		{
			if(RequestGetArgCount(request) != 3) return 0; 

			int originID = atoi(RequestGetArg(request, 1));
			int searchID = atoi(RequestGetArg(request, 2));

			if(RingManagerCheck(server->ringmanager, searchID)){
				RingManagerRsp(server->ringmanager, originID, searchID, RingManagerId(server->ringmanager), server->ip, server->TCPport);
			}else{
				RingManagerQuery(server->ringmanager, originID, searchID );
			}
			
		}
		default:
			//Handle unrecognized command
			break;
	}
	
	return 0;
}

/* Process TCP Manager Requests */

int ServerProcTCPReq(Server * server, Request * request){
	int argCount = RequestGetArgCount(request);
	if(argCount <= 0) return 0;

	printf("External wrote: ");
	int i = 0;
  for(i = 0; i<RequestGetArgCount(request); i++){
    printf("%s ", RequestGetArg(request, i));
    fflush(stdout);
	} 
	printf("\n");
	
	char * command = RequestGetArg(request,0);
	int code = hash(command);
	
	switch(code){
		case(TCP_NEW):
		{
			if(RequestGetArgCount(request) != 4) break;
			
			int originID    = atoi(RequestGetArg(request, 1));
			char * originIP = RequestGetArg(request, 2);
			int originPort  = atoi(RequestGetArg(request, 3));
			
			RingManagerNew(server->ringmanager, RequestGetFD(request), originID, originIP, originPort);
			TCPManagerRemoveSocket(server->tcpmanager, RequestGetFD(request));
			
			break;
		}
		case(TCP_CON):
		{
			int id   = RingManagerId(server->ringmanager);
			int succiID    = atoi(RequestGetArg(request, 1));
			char * succiIP = RequestGetArg(request, 2); 
			int succiPort = atoi(RequestGetArg(request, 3)); 
                    
			RingManagerConnect(server->ringmanager, 1, id, succiID, succiIP, succiPort);
			
			break;	
		}
		case(TCP_SUCC):
		{
			int destID = atoi(RequestGetArg(request, 1));
			char * destIP   = RequestGetArg(request, 2); 
			int destPort    = atoi(RequestGetArg(request, 3)); 

			if(UDPManagerID(server->udpmanager) == destID){
				printf("ID %d already in use in ring, please select another\n", destID);
				break;
        close(UDPManagerID(server->udpmanager);
			}
			
			close(TCPManagerIDfd(server->tcpmanager));
			TCPManagerSetSearch(server->tcpmanager, -1, -1);
			
			RingManagerConnect(server->ringmanager,
							UDPManagerRing(server->udpmanager), 
							UDPManagerID(server->udpmanager), 
							destID, destIP, destPort);
			break;
		}
		case(TCP_ID):
		{
			if(RequestGetArgCount(request) != 2) break;
			int search = atoi(RequestGetArg(request, 1));
			int id = RingManagerId(server->ringmanager);
			if(RingManagerCheck(server->ringmanager, search)){
				char msg[50];
				sprintf(msg, "SUCC %d %s %d\n", UDPManagerID(server->udpmanager), server->ip, server->TCPport);
				write(TCPManagerIDfd(server->tcpmanager), msg, strlen(msg));
			}
			else{
				RingManagerQuery(server->ringmanager, id, search);
				TCPManagerSetSearch(server->tcpmanager, RequestGetFD(request), search);
			}
		}
		
		default: 
			break;
	}

	return 1;
}

/* Process UI Manager Requests */

int ServerProcUIReq(Server * server, Request * request){
	
	int argCount = RequestGetArgCount(request);
	if(argCount <= 0) return 0;

	char * command = RequestGetArg(request,0);
	int code = hash(command);
	
	switch(code){
		case(UI_JOIN):
		{
			if(argCount == 3){
				/*Send UDP BQRY x*/
				/*if(BQRY == EMPTY)*/
				/*RingManagerSetId(server->ringmanager, atoi(RequestGetArg(request, 2)));*/
				UDPManagerJoin(server->udpmanager, atoi(RequestGetArg(request, 1)), atoi(RequestGetArg(request, 2)));
				/*UDPManagerReg(server->udpmanager, atoi(RequestGetArg(request, 1)), atoi(RequestGetArg(request, 2)));*/
				/*else send(ID)*/
				/*receive "SUCC" from succi*/
			}else if(argCount == 6){
			
				int ring = atoi(RequestGetArg(request, 1));
				int id   = atoi(RequestGetArg(request, 2));
				int succiID    = atoi(RequestGetArg(request, 3));
				char * succiIP = RequestGetArg(request, 4); 
				int succiPort  = atoi(RequestGetArg(request, 5)); 

				RingManagerConnect(server->ringmanager, ring, id, succiID, succiIP, succiPort);
			}
			break;
		}
		case(UI_LEAVE):
			/* To do: Check if only one in ring, if so,
			 * tell boot server to remove node. Else:
			 * REG x succi to boot server and*/
			RingManagerLeave(server->ringmanager);
			break;	
		case(UI_SHOW):
			RingManagerStatus(server->ringmanager);
			break;
		case(UI_RSP):
			RingManagerRsp(server->ringmanager, 0, 1, RingManagerId(server->ringmanager), server->ip, server->TCPport);
			break;	
		case(UI_SEARCH):
		{
			/*Reminder: limit commands if user is not connect to ring*/
			if(RequestGetArgCount(request) < 2) return 0;

			int search = atoi(RequestGetArg(request, 1));
			int id     = RingManagerId(server->ringmanager);

			if( RingManagerCheck(server->ringmanager, search) ){
				printf("Yey, don't have to go far: %i, ip, port\n", id); /*Add variables for ip and port eventually*/
			} else {
				RingManagerQuery(server->ringmanager, id, search); /*Add int->string support eventually*/
			}
			break;
		}
		case(UI_BOOPP):
			RingManagerMsg(server->ringmanager, 1, "Boop\nBoop\n");
			
			break;
		case(UI_BOOPS):
			RingManagerMsg(server->ringmanager, 0, "Boop\nBoop\n");
			break;
		case(UI_STREAM):
		{
			char buffer[100];
			fgets(buffer, 100, stdin);
			buffer[strlen(buffer)-1] = '\0';
			
			RingManagerMsg(server->ringmanager, 1, buffer);
			break;
		}
		case (UI_CLOSESTREAM):
			RingManagerMsg(server->ringmanager, 1, "\n");
			break;	
		case(UI_HASH):
			printf("hash: %i\n", hash(RequestGetArg(request, 1)));
			break;
		case(UI_SEND):
		{
			char buffer[100];
			fgets(buffer, 100, stdin);
			
			RingManagerMsg(server->ringmanager, 1, buffer);
			break;
		}
		case(UI_EXIT):
			server->shutdown = 1;
			break;
		case(UI_CHECK):
		{			
			int id = atoi(RequestGetArg(request, 1));
			
			if(RingManagerCheck(server->ringmanager, id)){
				printf("It's ours");
			} else {
				printf("No luck here");
			}
			
			fflush(stdout);
			break;
		}
		default:
			printf("Comando não reconhecido\n");
			break;
	}
	
	return 0;
}

void ServerSetIP(Server * server, char* ip){
	strcpy(server->ip,ip);
}

int hash(char *str)
{
    int h = 0;
    while (*str)
       h = h << 1 ^ *str++;
    return h;
}

int ServerProcArg(Server * server, int argc, char ** argv){
	
	char * ringPort = NULL;
	char * bootIP 	= NULL;
	char * bootPort = NULL;
	int i, opt;

	opterr = 0;
	while ((opt = getopt(argc, argv, "t:i:p:")) != -1){
		switch (opt){					/*I'm sure you can probably clean up this switch somehow, i'll leave that to you*/
			case 't':
				ringPort = optarg;
				break;
			case 'i':
				bootIP = optarg;
				break;
			case 'p':
				bootPort = optarg;
				break;
			case '?':
				if (optopt == 't')
					fprintf(stderr, "Opcao -%c requer argumento.\n", optopt);
				if (optopt == 'i')
					fprintf(stderr, "Opcao -%c requer argumento.\n", optopt);
				if (optopt == 'p')
					fprintf(stderr, "Opcao -%c requer argumento.\n", optopt);
				else if(isprint(optopt))
					fprintf(stderr, "Opcao desconhecida '-%c'. Sintaxe: ddt [-t ringport] [-i bootIP] [-p bootport]\n", optopt);
				else
					fprintf(stderr, "Caracter de opcao desconhecido '\\x%x'.\n", optopt);
				return 0;
			default:
				return 0;
		}
  }
  
	printf("tvalue = %s, ivalue = %s, pvalue = %s\n", ringPort, bootIP, bootPort);

	for (i = optind; i < argc; i++)
		fprintf (stderr, "Argumento invalido %s\n", argv[i]);
	
	if(ringPort != NULL)	server->TCPport = atoi(ringPort);
	if(bootIP   != NULL)	UDPManagerSetIP(server->udpmanager, bootIP);
	if(bootPort != NULL)	UDPManagerSetPort(server->udpmanager, atoi(bootPort));

	return 0;	
}
