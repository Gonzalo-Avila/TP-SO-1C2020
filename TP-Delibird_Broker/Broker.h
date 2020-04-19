/*
 * Broker.h
 *
 *  Created on: 10 abr. 2020
 *      Author: utnso
 */

#ifndef BROKER_H_
#define BROKER_H_

#include <Utils.h>



t_queue * NEW_POKEMON;
t_queue * APPEARED_POKEMON;
t_queue * CATCH_POKEMON;
t_queue * CAUGHT_POKEMON;
t_queue * GET_POKEMON;
t_queue * LOCALIZED_POKEMON;

t_list * suscriptoresNEW;
t_list * suscriptoresAPP;
t_list * suscriptoresCAT;
t_list * suscriptoresCAU;
t_list * suscriptoresGET;
t_list * suscriptoresLOC;

void * cacheBroker;
int CACHESIZE;

typedef struct {
	int id;
	int idCorrelativo;			// Si no se usa idCorrelativo = -1
	t_list *listaSuscriptores;
	void* mensaje;
	int sizeMensaje;
} estructuraMensaje;


#endif /* BROKER_H_ */
