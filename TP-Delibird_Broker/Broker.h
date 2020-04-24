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



typedef enum{
	NUEVO=0,
    ENVIADO=1,
	CONFIRMADO=2
}statusMensaje;

int generarID();
t_list* getListaSuscriptoresByNum(int nro);

#endif /* BROKER_H_ */
