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

t_queue * NEW_POKEMON;
t_queue * APPEARED_POKEMON;
t_queue * CATCH_POKEMON;
t_queue * CAUGHT_POKEMON;
t_queue * GET_POKEMON;
t_queue * LOCALIZED_POKEMON;

typedef enum
{
	MENSAJE=1
}opCode;

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

int test(void);
int crearConexionServer(char * puerto);
int crearConexionCliente(char * puerto, char * ip);
int * esperarCliente(int socketEscucha, int backlog);
void inicializarColas();
void * serializarPaquete(tPaquete* paquete, int tamanioAEnviar);
void enviarMensaje(char * mensaje, int socketDestino);
tPaquete *recibirMensaje(int socketFuente);

#endif /* UTILS_H_ */
