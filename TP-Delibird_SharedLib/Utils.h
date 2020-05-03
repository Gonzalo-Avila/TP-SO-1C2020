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
#include <math.h>
#include <commons/string.h>
t_log * logger;
t_config * config;

/*	<opCode><Type><Msj>
 * SUSCRIPCION 	<nombreCola> 							= 3 1
 * MENSAJE 		NEW 		<MsjLenght> <NombrePokemon> = 1 1 <MsjLenght> <NombrePokemon>
 * MENSAJE 		APPEARED  	<MsjLenght> <NombrePokemon> = 1 2 <MsjLenght> <NombrePokemon>
 * MENSAJE 		CATCH 	 	<MsjLenght> <NombrePokemon> = 1 3 <MsjLenght> <NombrePokemon>
 * MENSAJE 		CAUGHT 	 	<MsjLenght> <NombrePokemon> = 1 4 <MsjLenght> <NombrePokemon>
 * MENSAJE 		GET 	 	<MsjLenght> <NombrePokemon> = 1 5 <MsjLenght> <NombrePokemon>
 * MENSAJE 		LOCALIZED	<MsjLenght> <NombrePokemon> = 1 6 <MsjLenght> <NombrePokemon>
 * */

typedef enum
{
	NUEVO_MENSAJE=1,
	FINALIZAR=2,
	SUSCRIPCION=3,
	CONFIRMACION_MENSAJE=4
}opCode;

typedef enum
{
	NEW=0,
	APPEARED=1,
	CATCH=2,
	CAUGHT=3,
	GET=4,
	LOCALIZED=5
}cola;

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
	uint32_t longPokemon;   //Longitud del nombre del pokemon.
	char * pokemon;         //Nombre del pokemon a agregar.
	uint32_t posicionX;
	uint32_t posicionY;;  //Posicion del pokemon en el mapa. Primer componente fila, segundo componente columna.
    uint32_t cantPokemon;   //Cantidad pokemons a agregar en la posición.
}mensajeNew;

//Este mensaje avisa que ha aparecido un nuevo pokemon en la posicion indicada
typedef struct
{
	uint32_t longPokemon;   //Longitud del nombre del pokemon.
	char * pokemon;         //Nombre del pokemon que apareció.
	uint32_t posicionX;
	uint32_t posicionY;  //Posicion del pokemon en el mapa. Primer componente fila, segundo componente columna.
}mensajeAppeared;

//Este mensaje indica que se va a atrapar un pokemon en determinada posicion
typedef struct
{
	uint32_t longPokemon;   //Longitud del nombre del pokemon.
	char * pokemon;         //Nombre del pokemon a atrapar.
	uint32_t posicionX;
	uint32_t posicionY;;  //Posicion del pokemon en el mapa. Primer componente fila, segundo componente columna.
}mensajeCatch;

//Este mensaje confirma si el resultado de la operacion "Catch" fue correcto o fallo
typedef struct
{
    uint32_t resultado;     //Define si se atrapo correctamente o no. Habria que ver que tipo de variable seria.
}mensajeCaught;

//Este mensaje solicita todas las posiciones en las que se puede encontrar determinado pokemon
typedef struct
{
	uint32_t longPokemon;   //Longitud del nombre del pokemon.
	char * pokemon;         //Nombre del pokemon cuyas posiciones se desea conocer.
}mensajeGet;

//Este mensaje informa todas las posiciones donde se puede encontrar un pokemon, y en que cantidades
typedef struct
{
	uint32_t longPokemon;   //Longitud del nombre del pokemon.
	char * pokemon;         //Nombre del pokemon cuyas posiciones se esta informando.
    uint32_t listSize;
    t_list * posicionYCant; //Lista de todas las posiciones donde esta el pokemon y cantidad en cada una, seria un struct.
}mensajeLocalized;

//Estructura para las componentes de la lista de posiciones y cantidades
typedef struct
{
	uint32_t posicionX;
	uint32_t posicionY;   //Posicion del pokemon en el mapa. Primer componente fila, segundo componente columna.
	uint32_t cantidad;      //Cantidad de pokemons que hay en la posición.
}posicYCant;

typedef struct {
	int id;
	int idCorrelativo;			// Si no se usa idCorrelativo = -1
	t_list *listaSuscriptores;
	int sizeMensaje;
	void* mensaje;
} estructuraMensaje;

typedef struct {
	opCode codeOP;
	int sizePayload;
	cola colaEmisora;
	int idMensaje;
	int idCorrelativo;
    int sizeMensaje;
    void * mensaje;
}mensajeRecibido;


void atenderConexionEn(int socket, int backlog);
int crearConexionServer(char * ip, char * puerto);
int crearConexionCliente(char * ip, char * puerto);
int esperarCliente(int socketEscucha);
void inicializarColas();
void * serializarPaquete(tPaquete* paquete, int tamanioAEnviar);
void * serializarPaqueteCola(tPaquete* paquete, int tamanioAEnviar);
void enviarString(int socketDestino, char * mensaje);
void enviarMensajeACola(int socketDestino, cola tipoCola, char * mensaje);
void enviarMensajeASuscriptor(int socketSuscriptor,cola colaEmisora, estructuraMensaje datosMensaje);
void enviarMensajeABroker(int socketBroker, cola colaDestino,int idCorrelativo,int sizeMensaje, void * mensaje);
mensajeRecibido recibirMensajeDeBroker(int socketFuente);
void loggearMensaje(t_log *logger,char * mensaje);
int test();
void suscribirseACola(int socketBroker, cola tipoCola);
void enviarACola(int socketBroker, cola tipoCola, char* msj, int msjSize);

#endif /* UTILS_H_ */
