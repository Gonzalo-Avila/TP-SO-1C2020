/*
 * Team.h
 *
 *  Created on: 10 abr. 2020
 *      Author: utnso
 */

#ifndef TEAM_H_
#define TEAM_H_

#include <Utils.h>

const MAXSIZE = 1024;

typedef int t_posicion[2];

typedef struct {
	t_list *entrenadores;
	char *algoritmoPlanificacion;
}t_team;

typedef struct{
	t_posicion pos;
	t_list *objetivos;
	t_list *pokemones;
}t_entrenador;

typedef enum{
	FIFO,	//First In First Out
	RR,		//Round Robin
	SJFCD,	//Shortest Job First Con Desalojo
	SJFSD	//Shortest Job First Sin Desalojo
}e_algoritmo;

#endif /* TEAM_H_ */
