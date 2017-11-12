#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

#define NTR  "\x1B[0m"
#define VML  "\x1B[31m"
#define VRD  "\x1B[32m"
#define AMA  "\x1B[33m"
#define AZU  "\x1B[34m"

//Tamanho maximo da mensagem
#define TMAX_MSG 1024

//------- Variáveis globais ----
struct sockaddr_in servidor;
int _socket;
int porta;
int tSocket;
pthread_t* threadCliente;
int n_clientes = 0;
int n_thread = 1;
char* alerta;

//------- Estrutura de cada cliente
struct _clientes{
	
	//Nome do cliente
	//Estrutura de socket do cliente
	//Numero identificador do socket cliente
	//Numero identificador da thread que executa o cliente
	//Ponteiro para o proximo cliente (lista encadeada simples)

	char nome[50];
	struct sockaddr_in clienteStruct;
	int socketCliente;
	int threadId;
	struct _clientes *prox;

};

struct _clientes* _clienteInit = NULL;

void telaAjuda(){
	printf("Trabalho de programação - REDES II\n");
	printf("Comandos:\n");
	printf("\t%s/listar%s - Lista todos os usuarios\n", AMA, NTR);
	printf("\t%s/r {id}%s - Encerra conexao do usuario {id} e remove da lista\n", AMA, NTR);
	printf("\t%s/encerrar%s - Fecha todas as conexões e encerra o servidor\n", AMA, NTR);
	printf("--------------------------------------------------------------\n");
}

//Funcao de busca recursiva ao ultimo cliente
struct _clientes* buscaUltimo(struct _clientes* cliente){

	if (cliente->prox != NULL)
		cliente = buscaUltimo(cliente->prox);

	return cliente;
}

//Adiciona cliente a lista encadeada
void adicionarCliente(struct _clientes* c){

	struct _clientes* cl;
	c->prox = NULL;

	if (!n_clientes){
		_clienteInit = c;
		_clienteInit->prox = NULL;
	}
	else{
		cl = buscaUltimo(_clienteInit);
		cl->prox = c;

	}

	n_clientes++;
}

//Obtem o destinatário de uma mensagem privada
//Retorna um vetor contendo o destinatário e o indice onde se inicia a mensagem útil
int* obtemDestinatario(char* mensagem){
	
	if (strlen(mensagem) < 3)
		return 0;


	int* i = (int*) malloc(sizeof(int) * 2);
	
	i[0] = 0;

	for(i[1] = 3; i[1] < strlen(mensagem); i[1]++){

		if (mensagem[i[1]] < 48 || mensagem[i[1]] > 57)
			return i;
		
		i[0] *= 10;
		i[0] += mensagem[i[1]] - 48;
	}

	printf("obtemDistinatario\n i[0] = %d i[1] = %d", i[0],i[1]);

	return i;
}

//Obtem a mensagem ocultado as partes iniciais da mensagem
char* obtemMensagem(char* mensagem, int i){
	
	char* aux = (char*) calloc(TMAX_MSG, 1);
	int j = i;
	for(i; i < strlen(mensagem); i++)
		aux[i - j] = mensagem[i];

	bzero(mensagem, strlen(mensagem));

	return aux;
}

//Retorna o Endereco IP em formato de texto
char* getIP(struct sockaddr_in cliente){

	char* ip = (char*) malloc(15);

	sprintf(ip, "%d.%d.%d.%d",
		cliente.sin_addr.s_addr 	& 0x000000ff,
		(cliente.sin_addr.s_addr 	& 0x0000ff00) >> 8,
		(cliente.sin_addr.s_addr 	& 0x00ff0000) >> 16,
		(cliente.sin_addr.s_addr 	& 0xff000000) >> 24);

	return ip;
}

//Funcao de busca recursiva do cliente, retorna a 
//estrutura (ou celula) do cliente pesquisado
struct _clientes* obterCliente(int idSocket){

	struct _clientes* cl = _clienteInit;

	do{
		if (cl->socketCliente == idSocket)
			return cl;

		cl = cl->prox;

	}while(cl != NULL);

	return cl;
}

//Mostra a lista de clientes na tela
void mostraLista(struct _clientes* cliente){

	if (cliente){
		printf("ID: %d | IP: %s | Nome: %s\n", cliente->socketCliente, getIP(cliente->clienteStruct), cliente->nome);
		if(cliente->prox)
			mostraLista(cliente->prox);
	}

}

//Escreve a lista de clientes no endereco em argumento
void obterListadeusuarios(char* buffer){

	bzero(buffer, TMAX_MSG);
	struct _clientes* cl = _clienteInit;
	
	sprintf(buffer, "\t%s>>LISTA DE CLIENTES<<%s\n", VML, NTR);

	while(cl != NULL){

		sprintf(buffer, "%s\tID:%d | IP: %s | Nome: %s\n", buffer,  cl->socketCliente, getIP(cl->clienteStruct), cl->nome);
		cl = cl->prox;
	}
	
}

//Envia mensagem publica (para todos os clientes) exceto ao remetente
void mensagemPublica(struct _clientes* cl, char* mensagem){

	struct _clientes* aux = _clienteInit;

	do{
		if (aux->socketCliente != cl->socketCliente)
			send(aux->socketCliente, mensagem, strlen(mensagem) + 1, 0);

		aux = aux->prox;

	}while(aux != NULL);

}

//Envia mensagem privada ao cliente especificado
void mensagemPrivada(int destino, char* mensagem){
		send(destino, mensagem, strlen(mensagem) + 1, 0);
}

//Encapsula a mensagem adicionando um cabecalho com o nome do rementente.
// -- mensagem = "[" + nome "]: " + buffer
char* encapsularMensagem(char* nome, char* buffer){

	char* mensagem = (char*) calloc(TMAX_MSG, 1);
	strcat(mensagem, "[");
	strcat(mensagem, VRD);
	strcat(mensagem, nome);
	strcat(mensagem, NTR);
	strcat(mensagem, "]: ");
	strcat(mensagem, buffer);

	return mensagem;
}

//Remover cliente com base no identificador (socket)
int removerCliente(int id){

	if (_clienteInit->socketCliente == id){

		send(_clienteInit->socketCliente, alerta, strlen(alerta) + 1, 0);
		close(_clienteInit->socketCliente);
		pthread_cancel(threadCliente[_clienteInit->threadId]);

		if (_clienteInit->prox == NULL)
			_clienteInit = NULL;
		else{
			struct _clientes* aux = _clienteInit->prox;
			free(_clienteInit);
			_clienteInit = aux;
		}

		n_clientes--;

		return 1;

	}else if (_clienteInit->prox){

		struct _clientes* cl = _clienteInit;
		struct _clientes* aux = (struct _clientes*) malloc(sizeof(struct _clientes));

		do{
			aux = cl->prox;

			if (aux->socketCliente == id){

				send(_clienteInit->socketCliente, alerta, strlen(alerta) + 1, 0);
				bzero(alerta, 50);
				strcpy(alerta, "\1\2\3\n");
				send(_clienteInit->socketCliente, alerta, strlen(alerta) + 1, 0);

				close(id);
				pthread_cancel(threadCliente[aux->threadId]);

				cl->prox = aux->prox;
				free(aux);
				n_clientes--;

				return 1;
			}

			cl = cl->prox;

		}while(cl != NULL);
	}

	printf("%s>> CLIENTE DESCONHECIDO <<%s\n", VML, NTR);
	return 0;
}

//Envia mensagens de aviso 
//fecha as conexoes de todos os usuarios
void fecharConexoes(struct _clientes *cl){
	
	if (cl == NULL)
		return;

	send(cl->socketCliente, alerta, strlen(alerta) + 1, 0);
	
	bzero(alerta, 50);
	strcpy(alerta, "\1\2\3\n");
	send(_clienteInit->socketCliente, alerta, strlen(alerta) + 1, 0);
	
	sleep(1);

	close(cl->socketCliente);
	printf("Conexão %d encerrada!\n", cl->socketCliente);
	fecharConexoes(cl->prox);
}

//Função para a trhead de gerenciamento de clientes (recebe mensagens)
void* clienteHandler(void* cliente_){

	pthread_t threadEnvio;

	struct _clientes* cliente = cliente_;
	int socketCliente = cliente->socketCliente, i;

	socklen_t tCliente = sizeof(cliente->clienteStruct);

	getpeername(socketCliente, (struct sockaddr*)&cliente->clienteStruct, &tCliente);

	char* pct_buffer = (char*) calloc(8 * sizeof(char), 1);
	char* mensagem = (char*) calloc(TMAX_MSG, 1);

	//Envia mensagens publicas quando algum cliente "entra"
	if (n_clientes > 1) {
		sprintf(mensagem, "Cliente %s%s%s (%s) Conectado!\n", VRD, getIP(cliente->clienteStruct), NTR, cliente->nome);
		mensagemPublica(cliente, mensagem);
		bzero(mensagem, 1024);
	}

	while(recv(socketCliente, pct_buffer, sizeof(pct_buffer), 0) > 0){

		strcat(mensagem, pct_buffer);

		for(i = 0; i < sizeof(pct_buffer); i++){

			if (pct_buffer[i] == 10){

				if (mensagem[0] == 47){

					//Flag de remetente
					//Servidor se igual à 1
					//Nome do cliente se igual a 0
					int j = 0;	

					//Verifica equidade com o comando sair
					//e então finaliza o cliente
					if (!strcmp(mensagem, "/sair\n")){
						
						strcpy(mensagem, cliente->nome);
						strcat(mensagem, " Desconectou-se!\n");
						strcpy(mensagem, encapsularMensagem("Servidor", mensagem));
			
						mensagemPublica(cliente, mensagem);
						free(mensagem);
						removerCliente(socketCliente);
						
						close(socketCliente);

						pthread_exit(NULL);

					}
					else if (!strcmp(mensagem, "/listar\n")){
						
						bzero(mensagem, strlen(mensagem) + 1);
						obterListadeusuarios(mensagem);
						j = 1;

					}else if (mensagem[1] == 'p' && mensagem[2] == ' '){
						
						//Pressupõe-se que após a sequência /p\32 os próximos caracteres sejam
						//o argumento de {destinatario} (é necessario verificar antes se é composto somente por números)
						int* vetor = obtemDestinatario(mensagem);

						if (vetor[0] == 0){

							bzero(mensagem, strlen(mensagem) + 1);
							strcpy(mensagem, "Destinatário inválido!");
							j = 1;

						}else{

							if (obterCliente(vetor[0])){

								//Passado no teste de verificação.
								//Obtem a mensagem contida apos o argumento {destinatario}
								mensagem = obtemMensagem(mensagem, vetor[1]);
								strcpy(mensagem, encapsularMensagem(cliente->nome, mensagem));
								char* aux = (char*) malloc(15);
								
								//Encapsula a mensagem novamente, deixando explícito
								//que é uma mensagem privada
								sprintf(aux, "[%sPrivado%s]", VML, NTR);
								strcpy(mensagem, encapsularMensagem(aux, mensagem));
								
								free(aux);
								mensagemPrivada(vetor[0], mensagem);
								
							}else{

								bzero(mensagem, strlen(mensagem) + 1);
								strcpy(mensagem, "Destinatário inválido!");
								j = 1;
							}
						}
					}
					else{

						//Caso não seja um dos comandos esperados
						strcpy(mensagem, ">>COMANDO INVÁLIDO!<<\n");
						j = 1;
					}

					if (j) {

						//Encapsula a mensagem de resposta do servidor para as situações acima
						//e envia devolta ao remetente
						strcpy(mensagem, encapsularMensagem("Servidor", mensagem));
						mensagemPrivada(socketCliente, mensagem);
					}

				}else{
					
					//Mensagens publicas
					strcpy(mensagem, encapsularMensagem(cliente->nome, mensagem));
					mensagemPublica(cliente, mensagem);
				}
				
				//Limpa os endereços
				bzero(mensagem, strlen(mensagem));
				bzero(pct_buffer, 8);
			}
		}
	}

}

//Função responsável pelo controle conexoes.
void* controleConexoes(){

	char i, flag;

	while (1){

		char* nome = (char*) calloc(50, 1);
		flag = 1;

		int novoCliente = accept(_socket, (struct sockaddr*) &servidor, &tSocket);

		if (novoCliente < 0){
			perror("Erro: ");
			close(novoCliente);
		}

		struct _clientes* cliente = (struct _clientes*) malloc(sizeof(struct _clientes));
		(*cliente).socketCliente = novoCliente;
		(*cliente).prox = NULL;
		
		int j;

		while((j = recv((*cliente).socketCliente, nome, 50, 0)) > 0){

			strcat((*cliente).nome, nome);

			for(i = 0; i < strlen(cliente->nome); i++){
				if(nome[i] == 10){
					flag = 0;
					cliente->nome[strlen(cliente->nome) - 1] = 0;
					break;
				}
			}

			if (!flag) break;
		}

		(*cliente).threadId = n_thread - 1;
		n_thread++;

		adicionarCliente(cliente);

		pthread_create(&threadCliente[(*cliente).threadId], NULL, clienteHandler, cliente);
		threadCliente = (pthread_t*) realloc(threadCliente, sizeof(pthread_t) * n_thread);

		free(nome);
	}

	pthread_exit(NULL);
}

int main (int argc, char** argv){

	porta = 2017;
	char* comando = (char*) calloc(15, 1);

	pthread_t threadControle;
	threadCliente = (pthread_t*) malloc(sizeof(pthread_t));

	tSocket = sizeof(struct sockaddr_in);

	printf("Inicializando Servidor\n");

	_socket = socket(AF_INET, SOCK_STREAM, 0);

	servidor.sin_family = AF_INET;
	servidor.sin_addr.s_addr = INADDR_ANY;
	servidor.sin_port = htons(porta);

	memset(&(servidor.sin_zero), 0x00, sizeof(servidor.sin_zero));

	tSocket = sizeof(struct sockaddr_in);

	bind(_socket, (struct sockaddr*) &servidor, sizeof(servidor));

	listen(_socket, 1);
	printf("Porta: %d\n\n", porta);

	pthread_create(&threadControle, NULL, controleConexoes, NULL);

	alerta = (char*) calloc(50, 1);

	sprintf(alerta, "%sServidor fechando conexão!%s\n", VML, NTR);
	strcpy(alerta, encapsularMensagem("Servidor", alerta));

	while(1){

		fgets(comando, 15, stdin);

		//Ajuda sobre comandos
		if (!strcmp(comando, "/ajuda\n"))
			telaAjuda();

		//Comando para encerrar o servidor
		//Sintaxe: /encerrar
		//Fecha as conexoes com todos os clientes conectados
		if (!strcmp(comando, "/encerrar\n")){
				printf("Encerrando...\n");
				fecharConexoes(_clienteInit);
				pthread_cancel(threadControle);
				break;
		}

		//Comando para listar usuários
		//Sintaxe: /listar
		else if (!strcmp(comando, "/listar\n"))
			mostraLista(_clienteInit);

		//Comando para remover usuario
		//Sintaxe: /r {id}
		else if (comando[0] == '/' && comando[1] == 'r' && comando[2] == ' '){
			
			if (n_clientes){

				int* v = (int*) malloc(sizeof(int) * 2);
				v = obtemDestinatario(comando);
				
				printf("dest %d\n", v[0]);

				(removerCliente(v[0])) ?
					printf("Cliente %c removido!\n", v[0])
				:
					printf("Cliente não encontrado!\n");
			}
		}else
			telaAjuda();
	}

	int i;

	printf("Finalizando threads...\n");

	for(i = 0; i < sizeof(threadCliente)/sizeof(pthread_t); i++)
		pthread_join(threadCliente[i], NULL);

	return 0;
}
