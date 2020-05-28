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

void inicializarVariablesGlobales();
void esperarMensajesDeBrokerEnCola(int * socketSuscripcion);
void conectarYSuscribir(int * socketSuscripcionNEW, int * socketSuscripcionGET, int * socketSuscripcionCATCH,char * ipServidor,char * puertoServidor);



#endif /* GAME_CARD_H_ */
