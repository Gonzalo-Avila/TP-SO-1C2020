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

uint32_t idProceso;

char * ipServidor;
char * puertoServidor;
char * puntoDeMontaje;

pthread_t hiloEsperaNEW, hiloEsperaGET, hiloEsperaCATCH, hiloEsperaGameboy, hiloReconexiones;

typedef enum{
	NO_CONECTADO=0,
	CONECTADO=1,
	ERROR_CONEXION=-1
}estadoConexion;

estadoConexion statusConexionBroker;

int socketSuscripcionNEW, socketSuscripcionGET, socketSuscripcionCATCH, socketEscuchaGameboy;


//Game-Card.c
void inicializarVariablesGlobales();
void destruirVariablesGlobales();
void procesarNEW(mensajeRecibido * mensaje);
void procesarCATCH(mensajeRecibido * mensaje);
void procesarGET(mensajeRecibido * mensaje);
void enviarMensajeBroker(cola colaDestino, uint32_t idCorrelativo, uint32_t sizeMensaje, void * mensaje);
void inicializarFileSystem();
char * cadenasConcatenadas(char * cadena1, char * cadena2);

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
