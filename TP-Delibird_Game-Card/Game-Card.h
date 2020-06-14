/*
 * Game-Card.h
 *
 *  Created on: 10 abr. 2020
 *      Author: utnso
 */

#ifndef GAME_CARD_H_
#define GAME_CARD_H_

#include <Utils.h>

uint32_t idProceso;

char * ipServidor;
char * puertoServidor;

pthread_t hiloEsperaNEW, hiloEsperaGET, hiloEsperaCATCH, hiloEsperaGameboy, hiloReconexiones;

typedef enum{
	NO_CONECTADO=0,
	CONECTADO=1,
	ERROR_CONEXION=-1
}estadoConexion;

estadoConexion statusConexionBroker;

int socketSuscripcionNEW, socketSuscripcionGET, socketSuscripcionCATCH, socketEscuchaGameboy;

void inicializarVariablesGlobales();
void esperarMensajesDeBrokerEnCola(int * socketSuscripcion);
int conectarYSuscribir();

#endif /* GAME_CARD_H_ */
