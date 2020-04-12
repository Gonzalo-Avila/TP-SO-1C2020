/*
 * Utils.h
 *
 *  Created on: 6 abr. 2020
 *      Author: ripped dinos
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>
#include <commons/config.h>

t_queue * NEW_POKEMON;
t_queue * APPEARED_POKEMON;
t_queue * CATCH_POKEMON;
t_queue * CAUGHT_POKEMON;
t_queue * GET_POKEMON;
t_queue * LOCALIZED_POKEMON;

t_log * logger;
t_config * config;

typedef enum
{
	MENSAJE=1,
	FINALIZAR=2
}opCode;

typedef enum
{
	NEW=1,
	APPEARED=2,
	CATCH=3,
	CAUGHT=4,
	GET=5,
	LOCALIZED=6
}cola;

/*typedef struct
{
char * nombre;
int tamanioNombre;
char * dni;
int tamanioDNI;
}fruta;*/

typedef struct
{
	int size;//Cuanto pesa el STREAM
	void* stream;
} tBuffer;

typedef struct
{
	opCode codOperacion;
	tBuffer* buffer; //Buffer = size + stream
} tPaquete;

int crearConexionServer(char * puerto);
int crearConexionCliente(char * ip, char * puerto);
int * esperarCliente(int socketEscucha, int backlog);
void inicializarColas();
void * serializarPaquete(tPaquete* paquete, int tamanioAEnviar);
void enviarMensaje(char * mensaje, int socketDestino);
tPaquete *recibirMensaje(int socketFuente);
void loggearMensaje(t_log *logger,char * mensaje);
int test();


#endif /* UTILS_H_ */
