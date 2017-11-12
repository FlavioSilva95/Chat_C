#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define ML(var, tipo, t) tipo* var = (tipo*) malloc(t * sizeof(tipo))
#define CL(var, tipo, t) tipo* var = (tipo*) calloc(t, sizeof(tipo))

//Cores
#define NTR  "\x1B[0m"
#define VML  "\x1B[31m"
#define VRD  "\x1B[32m"
#define AMA  "\x1B[33m"

int flag_FIN = 0;
int _socket;
struct sockaddr_in servidor;

//----------
//Função responsavel por receber mensagens do servidor
//---------
void *recebeMensagens(void* arg){
	
	int i;

	ML(pct_buffer, char, 8);
	ML(buffer, char, 1024);

	while(recv(_socket, pct_buffer, sizeof(pct_buffer), 0) > 0){

		strcat(buffer, pct_buffer);

		for(i = 0; i < strlen(buffer) + 1; i++){

			if (buffer[i-1] == 10 && buffer[i] == 0){
				printf("%s", buffer);	
				
				if (!strcmp(buffer, "\1\2\3\n")){
					flag_FIN = 1;
					close(_socket);
					printf("Programa finalizado!\nPressione ENTER ou ^C para sair");
					pthread_exit(NULL);
				}

				memset(buffer, 0x00, 1024);
				memset(pct_buffer, 0x00, 8);
				break;
			}
		}
	}

	pthread_exit(NULL);
}

int main(int argc, char* argv[]){

	CL(nome, char, 50);
	CL(ip, char, 15);
	
	strcpy(ip, "127.0.0.1");
	int i, porta = 2017;

	//Permite passar o ip e a porta do servidor como argumentos
	//./cliente {ip} {porta}

	if (argc > 1)
		strcpy(ip, argv[1]);
	if (argc == 3)
		porta = atoi(argv[2]);

	pthread_t tRecebe;

	printf("Digite seu nome: ");
	fgets(nome, 50, stdin);

	_socket = socket(AF_INET, SOCK_STREAM, 0);
	servidor.sin_family = AF_INET;
	servidor.sin_addr.s_addr = inet_addr(ip);
	servidor.sin_port = htons(porta);

 	memset(&(servidor.sin_zero), 0x00, sizeof(servidor.sin_zero));

	printf("Tentando conectar...");
	printf("IP: %s | Porta: %d\n", ip, porta);

	if(connect(_socket,  (struct sockaddr *)&servidor, sizeof(struct sockaddr_in)) != 0){
		perror("ERRO! ");
		close(_socket);
		return 1;
	}

	send(_socket, nome, strlen(nome) + 1, 0);
	nome[strlen(nome) - 1] = 0;
	
	//espera 1 segundo... 
	sleep(1);

	printf("%s---------- Conectado! ------------%s\n\n", VRD, NTR);

	pthread_create(&tRecebe, NULL, recebeMensagens, NULL);

	printf("Digite sua mensagem:\n ");
	
	int j, k = 0;
	//Envia mensagens ao servidor, aloca e le mensagens do buffer de entrada
	while(1){
		
		if (flag_FIN)
			break;

		char* mensagem = (char*) malloc(1024 * sizeof(char));

		fgets(mensagem, sizeof(mensagem), stdin);

		j = send(_socket, mensagem, strlen(mensagem) + 1, 0);
		k += (j - 2);

		if(mensagem[k] == 10){
			printf("\r[%s%s%s]: %s", VRD, nome, NTR, mensagem);
			k = 0;
		}

		if (!strcmp(mensagem, "/sair\n"))
			break;
	}

	pthread_cancel(tRecebe);
	close(_socket);

	return 0;
}