/*
 * Utils.c
 *
 *  Created on: 6 abr. 2020
 *      Author: ripped dinos
 */


#include "Utils.h"



/* Crea un socket de escucha para un servidor en X puerto
 * RECORDAR HACER EL CLOSE DEL LISTENNING SOCKET EN LA FUNCION CORRESPONDIENTE
 */
int crearConexionServer(char * puerto){
	    struct addrinfo hints;
		struct addrinfo *serverInfo;

		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_flags = AI_PASSIVE;
		hints.ai_socktype = SOCK_STREAM;

		getaddrinfo(NULL, puerto, &hints, &serverInfo);


		int socketEscucha;
		socketEscucha = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);


		bind(socketEscucha,serverInfo->ai_addr, serverInfo->ai_addrlen);
		freeaddrinfo(serverInfo);
		return socketEscucha;
}

/* Crea una conexión con el servidor en la IP y puerto indicados, devolviendo el socket del servidor
 *
 */
int crearConexionCliente(char * ip, char * puerto){
		struct addrinfo hints;
		struct addrinfo *serverInfo;
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;

		getaddrinfo(ip, puerto, &hints, &serverInfo);

		int socketServidor;
		socketServidor = socket(serverInfo->ai_family, serverInfo->ai_socktype, serverInfo->ai_protocol);

		connect(socketServidor, serverInfo->ai_addr, serverInfo->ai_addrlen);
		freeaddrinfo(serverInfo);
		return socketServidor;

}

/* Espera un cliente y cuando recibe una conexion, devuelve el socket correspondiente al cliente conectado.
 * RECORDAR HACER EL FREE AL PUNTERO SOCKETCLIENTE EN LA FUNCIÓN CORRESPONDIENTE Y EL CLOSE AL SOCKET
 */
int * esperarCliente(int socketEscucha, int backlog)
{
	listen(socketEscucha, backlog);
	struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    int *socketCliente = (int*)malloc(sizeof(int));
	*socketCliente=accept(socketEscucha, (struct sockaddr *) &addr, &addrlen);
    return socketCliente;
}

/* Serializa un paquete de formato estandar (Código de operación / Tamaño de buffer / Stream)
 *
 */
void * serializarPaquete(tPaquete* paquete, int tamanioAEnviar)
{
	int offset=0;
	void* aEnviar=malloc(tamanioAEnviar);

	memcpy(aEnviar+offset,&(paquete->codOperacion),sizeof(int));
	offset+=sizeof(int);
    memcpy(aEnviar+offset,&(paquete->buffer->size),sizeof(int));
    offset+=sizeof(int);
    memcpy(aEnviar+offset,paquete->buffer->stream,paquete->buffer->size);

    return aEnviar;
}

/* Envía un string al socket destino
 *
 */
void enviarMensaje(char * mensaje, int socketDestino){

	int longMensaje = strlen(mensaje);
    tBuffer *buffer = malloc(sizeof(tBuffer));
    tPaquete *paquete = malloc(sizeof(tPaquete));

    buffer->size = longMensaje+1;
    buffer->stream = malloc(buffer->size);

    memcpy(buffer->stream,mensaje,buffer->size);

    paquete->buffer=buffer;
    if(strcmp(mensaje,"exit")==0)
    	paquete->codOperacion=FINALIZAR;
    else
        paquete->codOperacion=MENSAJE;

    int tamanioAEnviar = 2*sizeof(int)+buffer->size;
    void* aEnviar = serializarPaquete(paquete,tamanioAEnviar);

    send(socketDestino,aEnviar,tamanioAEnviar,0);

    free(buffer->stream);
    free(buffer);
    free(paquete);
    free(aEnviar);
}

/* Recibe un string enviado por el socket fuente
 *
 */
tPaquete *recibirMensaje(int socketFuente){

	tPaquete *paqueteRecibido = malloc(sizeof(tPaquete));
	paqueteRecibido->buffer=malloc(sizeof(tBuffer));

	recv(socketFuente,&(paqueteRecibido->codOperacion),sizeof(int),MSG_WAITALL);
	recv(socketFuente,&(paqueteRecibido->buffer->size),sizeof(int),MSG_WAITALL);
	paqueteRecibido->buffer->stream = malloc(paqueteRecibido->buffer->size);
	recv(socketFuente,paqueteRecibido->buffer->stream,paqueteRecibido->buffer->size, MSG_WAITALL);

	return paqueteRecibido;

}

/* Funcion de prueba
 *
 */
int test(){
  return 10;
}

/* Luego de creada la conexión con el broker, esta función envía el código de la cola a la que se va a suscribir.
 *
 */
void suscribirseACola(cola tipoCola, int socketBroker){
    send(socketBroker,(void*)(&tipoCola),sizeof(cola),0);
}



