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
#include <inttypes.h>


t_log * logger;
t_config * config;



typedef enum
{
	MENSAJE=1,
	FINALIZAR=2,
	SUSCRIPCION=3
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

//Este mensaje hace aparecer un nuevo pokemon en la posicion indicada
typedef struct
{
	uint32_t id;            //ID del mensaje. Entiendo que el emisor lo manda inicializado en 0,y el broker le asigna valor.
	uint32_t longPokemon;   //Longitud del nombre del pokemon.
	char * pokemon;         //Nombre del pokemon a agregar.
	uint32_t posicion [2];  //Posicion del pokemon en el mapa. Primer componente fila, segundo componente columna.
    uint32_t cantPokemon;   //Cantidad pokemons a agregar en la posición.
}mensajeNew;

//Este mensaje avisa que ha aparecido un nuevo pokemon en la posicion indicada
typedef struct
{
    uint32_t id;            //ID del mensaje.
	uint32_t idC;           //ID del mensaje al que responde (correlacional).
	uint32_t longPokemon;   //Longitud del nombre del pokemon.
	char * pokemon;         //Nombre del pokemon que apareció.
	uint32_t posicion [2];  //Posicion del pokemon en el mapa. Primer componente fila, segundo componente columna.
}mensajeAppeared;

//Este mensaje indica que se va a atrapar un pokemon en determinada posicion
typedef struct
{
	uint32_t id;            //ID del mensaje. Entiendo que el emisor lo manda inicializado en 0,y el broker le asigna valor.
	uint32_t longPokemon;   //Longitud del nombre del pokemon.
	char * pokemon;         //Nombre del pokemon a atrapar.
	uint32_t posicion [2];  //Posicion del pokemon en el mapa. Primer componente fila, segundo componente columna.
}mensajeCatch;

//Este mensaje confirma si el resultado de la operacion "Catch" fue correcto o fallo
typedef struct
{
    uint32_t id;            //ID del mensaje.
    uint32_t idC;           //ID del mensaje al que responde (correlacional).
    uint32_t resultado;     //Define si se atrapo correctamente o no. Habria que ver que tipo de variable seria.
}mensajeCaught;

//Este mensaje solicita todas las posiciones en las que se puede encontrar determinado pokemon
typedef struct
{
	uint32_t id;            //ID del mensaje. Entiendo que el emisor lo manda inicializado en 0,y el broker le asigna valor.
	uint32_t longPokemon;   //Longitud del nombre del pokemon.
	char * pokemon;         //Nombre del pokemon cuyas posiciones se desea conocer.
}mensajeGet;

//Este mensaje informa todas las posiciones donde se puede encontrar un pokemon, y en que cantidades
typedef struct
{
	uint32_t id;            //ID del mensaje.
	uint32_t idC;           //ID del mensaje al que responde (correlacional).
	uint32_t longPokemon;   //Longitud del nombre del pokemon.
	char * pokemon;         //Nombre del pokemon cuyas posiciones se esta informando.
    t_list * posicionYCant; //Lista de todas las posiciones donde esta el pokemon y cantidad en cada una, seria un struct.
}mensajeLocalized;

//Estructura para las componentes de la lista de posiciones y cantidades
typedef struct
{
	uint32_t posicion[2];   //Posicion del pokemon en el mapa. Primer componente fila, segundo componente columna.
	uint32_t cantidad;      //Cantidad de pokemons que hay en la posición.
}posicYCant;

void atenderConexionEn(int socket, int backlog);
int crearConexionServer(char * ip, char * puerto);
int crearConexionCliente(char * ip, char * puerto);
int esperarCliente(int socketEscucha);
void inicializarColas();
void * serializarPaquete(tPaquete* paquete, int tamanioAEnviar);
void enviarMensaje(char * mensaje, int socketDestino);
tPaquete *recibirMensaje(int socketFuente);
void loggearMensaje(t_log *logger,char * mensaje);
int test();
void suscribirseACola(cola tipoCola, int socketBroker);


#endif /* UTILS_H_ */
