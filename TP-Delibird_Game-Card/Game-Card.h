/*
 * Game-Card.h
 *
 *  Created on: 10 abr. 2020
 *      Author: utnso
 */

#ifndef GAME_CARD_H_
#define GAME_CARD_H_

#include <Utils.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <commons/bitarray.h>

//Interaccion con broker
//---------------------------------------------------------------------------------------------
typedef enum{
	NO_CONECTADO=0,
	CONECTADO=1,
	ERROR_CONEXION=-1
}estadoConexion;

char * ipServidor;
char * puertoServidor;
estadoConexion statusConexionBroker;
int socketSuscripcionNEW, socketSuscripcionGET, socketSuscripcionCATCH, socketEscuchaGameboy;
pthread_t hiloEsperaNEW, hiloEsperaGET, hiloEsperaCATCH, hiloEsperaGameboy, hiloReconexiones;
uint32_t idProceso;


//---------------------------------------------------------------------------------------------



//Gestion del FS
//---------------------------------------------------------------------------------------------
char * puntoDeMontaje;
uint32_t tamanioBloque, cantidadDeBloques, tiempoDeRetardo, tiempoDeReintentoDeAcceso;
t_bitarray * bitarrayBloques;
char * bitmap;
//---------------------------------------------------------------------------------------------



//Game-Card.c
void inicializarVariablesGlobales();
void destruirVariablesGlobales();
void procesarNEW(mensajeRecibido * mensaje);
void procesarCATCH(mensajeRecibido * mensaje);
void procesarGET(mensajeRecibido * mensaje);
int enviarMensajeBroker(cola colaDestino, uint32_t idCorrelativo, uint32_t sizeMensaje, void * mensaje);
void inicializarFileSystem();
char * cadenasConcatenadas(char * cadena1, char * cadena2);
bool existeElArchivo(char * rutaArchivo);
int buscarBloqueLibre();

//Game-Card_Conexiones.c
void esperarMensajesGameboy(int* socketSuscripcion);
void esperarMensajesBroker(int* socketSuscripcion);
int crearSuscripcionesBroker();
void atenderConexiones(int *socketEscucha);
estadoConexion conectarYSuscribir();
void mantenerConexionBroker();
void cerrarConexiones();
void crearConexionGameBoy();
void crearConexionBroker();

#endif /* GAME_CARD_H_ */
