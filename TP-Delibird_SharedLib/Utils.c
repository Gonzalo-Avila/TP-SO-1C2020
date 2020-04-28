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

	memcpy(aEnviar+offset,&(paquete->codOperacion),sizeof(opCode));
	offset+=sizeof(opCode);
    memcpy(aEnviar+offset,&(paquete->buffer->size),sizeof(int));
    offset+=sizeof(int);
    memcpy(aEnviar+offset,paquete->buffer->stream,paquete->buffer->size);

    return aEnviar;
}

//EDIT GONZALO 23/04
//------------------
/* Permite envíar un mensaje desde cualquier cliente, que el broker sepa interpretar. El size del mensaje que requiere como
 * parametro es el del contenido del struct GET, CATCH, etc., que se esta mandando.
 */
void enviarMensajeABroker(int socketBroker, cola colaDestino,uint32_t idCorrelativo,uint32_t sizeMensaje,
		                  void * mensaje){
	    int offset = 0;
	    int sizeTotal;
	    void *mensajeSerializado;


        tPaquete *paquete=malloc(sizeof(tPaquete));
        tBuffer *buffer = malloc(sizeof(tBuffer));
        buffer->stream=malloc(sizeMensaje);
        buffer->size=sizeMensaje+sizeof(cola)+sizeof(uint32_t)*2;

        paquete->codOperacion=NUEVO_MENSAJE;

        memcpy(buffer->stream+offset,&(colaDestino),sizeof(cola));
        offset+=sizeof(cola);
        memcpy(buffer->stream+offset,&(idCorrelativo),sizeof(uint32_t));
        offset+=sizeof(uint32_t);
        memcpy(buffer->stream+offset,&(sizeMensaje),sizeof(uint32_t));
        offset+=sizeof(uint32_t);

        switch(colaDestino){
          case NEW:{
              mensajeNew * msg = mensaje;
        	  memcpy(buffer->stream+offset,&(msg->longPokemon),sizeof(uint32_t));
        	  offset+=sizeof(uint32_t);
        	  memcpy(buffer->stream+offset,msg->pokemon,sizeof(msg->longPokemon));
              offset+=sizeof(msg->longPokemon);
              memcpy(buffer->stream+offset,&(msg->posicionX),sizeof(uint32_t));
              offset+=sizeof(uint32_t);
              memcpy(buffer->stream+offset,&(msg->posicionY),sizeof(uint32_t));
              offset+=sizeof(uint32_t);
              memcpy(buffer->stream+offset,&(msg->cantPokemon),sizeof(uint32_t));
              break;
          }
          case APPEARED:{
        	  mensajeAppeared * msg = mensaje;
        	  memcpy(buffer->stream+offset,&(msg->longPokemon),sizeof(uint32_t));
        	  offset+=sizeof(uint32_t);
        	  memcpy(buffer->stream+offset,msg->pokemon,sizeof(msg->longPokemon));
              offset+=sizeof(msg->longPokemon);
              memcpy(buffer->stream+offset,&(msg->posicionX),sizeof(uint32_t));
              offset+=sizeof(uint32_t);
              memcpy(buffer->stream+offset,&(msg->posicionY),sizeof(uint32_t));
              offset+=sizeof(uint32_t);
        	  break;
          }
          case CATCH:{
        	  mensajeCatch * msg = mensaje;
        	  memcpy(buffer->stream+offset,&(msg->longPokemon),sizeof(uint32_t));
        	  offset+=sizeof(uint32_t);
        	  memcpy(buffer->stream+offset,msg->pokemon,sizeof(msg->longPokemon));
              offset+=sizeof(msg->longPokemon);
              memcpy(buffer->stream+offset,&(msg->posicionX),sizeof(uint32_t));
              offset+=sizeof(uint32_t);
              memcpy(buffer->stream+offset,&(msg->posicionY),sizeof(uint32_t));
              offset+=sizeof(uint32_t);
        	  break;
          }
          case CAUGHT:{
        	  mensajeCaught * msg = mensaje;
        	  memcpy(buffer->stream+offset,&(msg->resultado),sizeof(uint32_t));
        	  offset+=sizeof(uint32_t);
        	  break;
          }
          case GET:{
              mensajeGet * msg = mensaje;
              memcpy(buffer->stream+offset,&(msg->longPokemon),sizeof(uint32_t));
              offset+=sizeof(uint32_t);
          	  memcpy(buffer->stream+offset,msg->pokemon,sizeof(msg->longPokemon));
              offset+=sizeof(msg->longPokemon);
        	  break;
          }
          case LOCALIZED:{
        	  mensajeLocalized * msg = mensaje;
        	  posicYCant variableAuxiliar;
        	  memcpy(buffer->stream+offset,&(msg->longPokemon),sizeof(uint32_t));
        	  offset+=sizeof(uint32_t);
  	    	  memcpy(buffer->stream+offset,msg->pokemon,sizeof(msg->longPokemon));
  	    	  offset+=sizeof(msg->longPokemon);
  	    	  memcpy(buffer->stream+offset,&(msg->listSize),sizeof(uint32_t));
  	    	  offset+=sizeof(uint32_t);
              for(int i=0;i<msg->listSize;i++)
              {
            	  variableAuxiliar=*(posicYCant *)(list_get(msg->posicionYCant,i));
            	  memcpy(buffer->stream+offset,&(variableAuxiliar.posicionX),sizeof(uint32_t));
            	  offset+=sizeof(uint32_t);
            	  memcpy(buffer->stream+offset,&(variableAuxiliar.posicionY),sizeof(uint32_t));
            	  offset+=sizeof(uint32_t);
            	  memcpy(buffer->stream+offset,&(variableAuxiliar.cantidad),sizeof(uint32_t));
            	  offset+=sizeof(uint32_t);
              }
        	  break;
          }
          default:{
        	  log_error(logger,"[ERROR]");
              log_error(logger,"No se pudo leer la cola destino");
          }
        }
        paquete->buffer=buffer;
        sizeTotal=buffer->size+sizeof(uint32_t)+sizeof(opCode);
        mensajeSerializado = serializarPaquete(paquete,sizeTotal);
        send(socketBroker,mensajeSerializado,sizeTotal,0);
        //free(mensaje); Esto crashea
        free(mensajeSerializado);
        free(paquete);
        //free(buffer->stream); Esto tambien
        free(buffer);
}

/* Permite enviar un mensaje a cualquier cliente, de forma que estos lo puedan interpretar
 *
 */
void enviarMensajeASuscriptor(int socketSuscriptor,cola colaEmisora, estructuraMensaje datosMensaje){

         tPaquete * paquete = malloc (sizeof(tPaquete));
         tBuffer * buffer = malloc (sizeof(tBuffer));
         void * paqueteSerializado;

         int offset=0;
         int sizeTotal;

         paquete->codOperacion=NUEVO_MENSAJE;

         buffer->size=sizeof(cola)+sizeof(uint32_t)*3+datosMensaje.sizeMensaje;
         buffer->stream=malloc(buffer->size);

         memcpy(buffer->stream+offset,&colaEmisora,sizeof(cola));
         offset+=sizeof(cola);
         memcpy(buffer->stream+offset,&(datosMensaje.id),sizeof(uint32_t));
         offset+=sizeof(uint32_t);
         memcpy(buffer->stream+offset,&(datosMensaje.idCorrelativo),sizeof(uint32_t));
         offset+=sizeof(uint32_t);
         memcpy(buffer->stream+offset,&(datosMensaje.sizeMensaje),sizeof(uint32_t));
         offset+=sizeof(uint32_t);
         memcpy(buffer->stream+offset,&(datosMensaje.mensaje),datosMensaje.sizeMensaje);

         paquete->buffer=buffer;

         sizeTotal=sizeof(opCode)+sizeof(uint32_t)+buffer->size;

         paqueteSerializado=serializarPaquete(paquete,sizeTotal);

         send(socketSuscriptor,paqueteSerializado,sizeTotal,0);

         free(paqueteSerializado);
         free(paquete);
         free(buffer->stream);
         free(buffer);



}

/* Recibe un mensaje del broker y lo guarda en un struct con formato mensajeRecibido
 *
 */
mensajeRecibido recibirMensajeDeBroker(int socketBroker){
	mensajeRecibido mensaje;
	recv(socketBroker,&(mensaje.codeOP),sizeof(opCode),MSG_WAITALL);
	recv(socketBroker,&(mensaje.sizePayload),sizeof(int),MSG_WAITALL);
	recv(socketBroker,&(mensaje.colaEmisora),sizeof(cola),MSG_WAITALL);
	recv(socketBroker,&(mensaje.idMensaje),sizeof(int),MSG_WAITALL);
	recv(socketBroker,&(mensaje.idCorrelativo),sizeof(int),MSG_WAITALL);
	recv(socketBroker,&(mensaje.sizeMensaje),sizeof(int),MSG_WAITALL);
	recv(socketBroker,mensaje.mensaje,mensaje.sizeMensaje,MSG_WAITALL);
	return mensaje;
}

/* Luego de creada la conexión con el broker, esta función envía el código de la cola a la que se va a suscribir.
 *
 */
void suscribirseACola(int socketBroker, cola tipoCola){

    tPaquete * paquete = malloc(sizeof(tPaquete));
    paquete->buffer=malloc(sizeof(tBuffer));
    paquete->buffer->stream=malloc(sizeof(cola));

    paquete->codOperacion=SUSCRIPCION;
    paquete->buffer->size=sizeof(cola);
    memcpy(paquete->buffer->stream,&(tipoCola),sizeof(cola));
    int sizeTotal = sizeof(opCode)+sizeof(cola)+sizeof(int);
    void * paqueteSerializado = serializarPaquete(paquete,sizeTotal);
    send(socketBroker,paqueteSerializado,sizeTotal,0);
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
    free(paqueteSerializado);
    log_info(logger,"Se ha suscrito a la cola %s",obtenerNombreCola(tipoCola));
}

//------------------

/* Envía un string al socket destino
 *
 */
void enviarString(int socketDestino, char * mensaje){

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
        paquete->codOperacion=NUEVO_MENSAJE;

    int tamanioAEnviar = 2*sizeof(int)+buffer->size;
    void* aEnviar = serializarPaquete(paquete,tamanioAEnviar);

    send(socketDestino,aEnviar,tamanioAEnviar,0);

    free(buffer->stream);
    free(buffer);
    free(paquete);
    free(aEnviar);
}

//Funciones auxiliares para logs
//---------------------------------------------------
char * obtenerNombreCola(cola tipoCola){
	char * cola = malloc(18);

     switch(tipoCola){
		 case NEW:{
			 strcpy(cola,"NEW_POKEMON");
			 break;
		 }
		 case APPEARED:{
			 strcpy(cola,"APPEARED_POKEMON");
			 break;
		 }
		 case CATCH:{
			 strcpy(cola,"CATCH_POKEMON");
			 break;
		 }
		 case CAUGHT:{
			 strcpy(cola,"CAUGHT_POKEMON");
			 break;
		 }
		 case GET:{
			 strcpy(cola,"GET_POKEMON");
			 break;
		 }
		 case LOCALIZED:{
			 strcpy(cola,"LOCALIZED_POKEMON");
			 break;
		 }
		 default:{
			 strcpy(cola,"UNDEFINED_QUEUE");
			 break;
		 }
     }
     return cola;
}

//---------------------------------------------------



/*
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
        paquete->codOperacion=NUEVO_MENSAJE;

    paquete->tipoCola = tipoCola;

    int tamanioAEnviar = sizeof(cola) + 2*sizeof(int)+buffer->size;
    void* aEnviar = serializarPaqueteCola(paquete, tamanioAEnviar);

    send(socketDestino,aEnviar,tamanioAEnviar,0);

    free(buffer->stream);
    free(buffer);
    free(paquete);
    free(aEnviar);
}
*/

/* Recibe un string enviado por el socket fuente
 * RECORDAR HACER LOS FREE CORRESPONDIENTES EN LA FUNCIÓN QUE LLAMA
 */
/*tPaquete *recibirMensaje(int socketFuente){

	tPaquete *paqueteRecibido = malloc(sizeof(tPaquete));
	paqueteRecibido->buffer=malloc(sizeof(tBuffer));

	recv(socketFuente,&(paqueteRecibido->codOperacion),sizeof(int),MSG_WAITALL);
	recv(socketFuente,&(paqueteRecibido->buffer->size),sizeof(int),MSG_WAITALL);
	paqueteRecibido->buffer->stream = malloc(paqueteRecibido->buffer->size);
	recv(socketFuente,paqueteRecibido->buffer->stream,paqueteRecibido->buffer->size, MSG_WAITALL);

	return paqueteRecibido;

}*/
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



