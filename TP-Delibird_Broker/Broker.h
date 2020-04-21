/*
 * Broker.h
 *
 *  Created on: 10 abr. 2020
 *      Author: utnso
 */

#ifndef BROKER_H_
#define BROKER_H_

#include <Utils.h>



t_list * NEW_POKEMON;
t_list * APPEARED_POKEMON;
t_list * CATCH_POKEMON;
t_list * CAUGHT_POKEMON;
t_list * GET_POKEMON;
t_list * LOCALIZED_POKEMON;

t_list * suscriptoresNEW;
t_list * suscriptoresAPP;
t_list * suscriptoresCAT;
t_list * suscriptoresCAU;
t_list * suscriptoresGET;
t_list * suscriptoresLOC;

t_list * IDs;

void * cacheBroker;
int CACHESIZE;

typedef struct {
	int id;
	int idCorrelativo;			// Si no se usa idCorrelativo = -1
	t_list *listaSuscriptores;
	int sizeMensaje;
	void* mensaje;
} estructuraMensaje;

typedef enum{
    NO_CONFIRMADO=0,
	CONFIRMADO=1
}statusMensaje;

int generarID();
t_list* getListaSuscriptoresByNum(int nro);

#endif /* BROKER_H_ */
