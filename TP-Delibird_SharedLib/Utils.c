/*
 * Utils.c
 *
 *  Created on: 6 abr. 2020
 *      Author: ripped dinos
 */


#include "Utils.h"

void atenderConexionEn(int socket, int backlog){
	listen(socket, backlog);
}

/* Crea un socket de escucha para un servidor en X puerto
 * RECORDAR HACER EL CLOSE DEL LISTENNING SOCKET EN LA FUNCION CORRESPONDIENTE
 */
int crearConexionServer(char * ip, char * puerto){

	    struct addrinfo hints;
		struct addrinfo *serverInfo;

		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_flags = AI_PASSIVE;
		hints.ai_socktype = SOCK_STREAM;

		getaddrinfo(ip, puerto, &hints, &serverInfo);


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
int esperarCliente(int socketEscucha)
{
	struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    int socketCliente;
    socketCliente=accept(socketEscucha, (struct sockaddr *) &addr, &addrlen);
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

void * serializarPaqueteCola(tPaqueteCola* paquete, int tamanioAEnviar)
{
	int offset=0;
	void* aEnviar=malloc(tamanioAEnviar);

	memcpy(aEnviar+offset,&(paquete->codOperacion),sizeof(int));
	offset+=sizeof(int);
	memcpy(aEnviar+offset,&(paquete->tipoCola),sizeof(int));
	offset+=sizeof(int);
    memcpy(aEnviar+offset,&(paquete->buffer->size),sizeof(int));
    offset+=sizeof(int);
    memcpy(aEnviar+offset,paquete->buffer->stream,paquete->buffer->size);

    return aEnviar;
}

/* Envía un string al socket destino
 *
 */
void enviarMensaje(int socketDestino, char * mensaje){

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

void enviarMensajeACola(int socketDestino, cola tipoCola, char * mensaje){

	int longMensaje = strlen(mensaje);
    tBuffer *buffer = malloc(sizeof(tBuffer));
    tPaqueteCola *paquete = malloc(sizeof(tPaqueteCola));

    buffer->size = longMensaje+1;
    buffer->stream = malloc(buffer->size);

    memcpy(buffer->stream, mensaje, buffer->size);

    paquete->buffer=buffer;
    if(strcmp(mensaje,"exit")==0)
    	paquete->codOperacion=FINALIZAR;
    else
        paquete->codOperacion=MENSAJE;

    paquete->tipoCola = tipoCola;

    int tamanioAEnviar = sizeof(cola) + 2*sizeof(int)+buffer->size;
    void* aEnviar = serializarPaqueteCola(paquete, tamanioAEnviar);

    send(socketDestino,aEnviar,tamanioAEnviar,0);

    free(buffer->stream);
    free(buffer);
    free(paquete);
    free(aEnviar);
}

/* Recibe un string enviado por el socket fuente
 * RECORDAR HACER LOS FREE CORRESPONDIENTES EN LA FUNCIÓN QUE LLAMA
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
 * Nota: no usa serialización, por lo que se mandan dos mensajes en lugar de uno, pero funciona. ¿Esta mal?
 */
void suscribirseACola(int socketBroker, cola tipoCola){
    opCode tipoMensaje = SUSCRIPCION;
	send(socketBroker,(void*)(&tipoMensaje),sizeof(opCode),0);
    send(socketBroker,(void*)(&tipoCola),sizeof(cola),0);
}

/*void enviarACola(int socketBroker, cola tipoCola, char* msj, int msjSize){
	int msjSizeReal = msjSize + 1;
	int size = sizeof(int) + sizeof(int) + msjSizeReal;
	void* aEnviar = malloc(size);
	memcpy(aEnviar, &tipoCola, sizeof(int));
	memcpy(aEnviar, &msjSize, sizeof(int));
	memcpy(aEnviar, msj, msjSizeReal);
	printf("%s", (char*) aEnviar);
	enviarMensaje(socketBroker, (char *) aEnviar);
}*/



