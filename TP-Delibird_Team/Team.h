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

#endif /* TEAM_H_ */
