/*
 * Team.h
 *
 *  Created on: 10 abr. 2020
 *      Author: utnso
 */

#ifndef TEAM_H_
#define TEAM_H_

#include <Utils.h>

#define MAXSIZE = 1024;

typedef int t_posicion[2];

typedef enum{
	FIFO,	//First In First Out
	RR,		//Round Robin
	SJFCD,	//Shortest Job First Con Desalojo
	SJFSD	//Shortest Job First Sin Desalojo
}e_algoritmo;

typedef enum{ //
	NUEVO,	  //
	LISTO,    //
	BLOQUEADO,//---> Estados del Entrenador
	EJEC,	  //
	FIN  	  //
}e_estado;

typedef struct {
	t_list *entrenadores;
	t_objetivo objetivos;
	e_algoritmo algoritmoPlanificacion;
}t_team;

typedef struct{
	t_list *pokemones;
	t_list *cantidades;
}t_objetivo;

typedef struct{
	int id;
	t_posicion pos;
	t_list *objetivos;
	t_list *pokemones;
	e_estado estado;
}t_entrenador;

t_team *team;
t_list *listaHilos;

#endif /* TEAM_H_ */
