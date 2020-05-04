/*
 * Team.h
 *
 *  Created on: 10 abr. 2020
 *      Author: utnso
 */

#ifndef TEAM_H_
#define TEAM_H_

#include <Utils.h>

#define MAXSIZE 1024

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
	t_list *objetivo;
	e_algoritmo algoritmoPlanificacion;
}t_team;

typedef struct{
	int id;
	t_posicion pos;
	t_list *objetivos;
	t_list *pokemones;
	e_estado estado;
}t_entrenador;

typedef struct{
	cola tipoDeMensaje;
	int pokemonSize;
	char* pokemon;
	int posicionX;
	int posicionY;
}t_mensaje;

typedef struct{
	pthread_t hilo;
	int idEntrenador;
}t_listaHilos;

typedef struct{
	float dist;
	int idEntrenador;
}t_dist;

t_team *team;
t_list *listaHilos;
t_queue *colaDeReady;
t_queue *colaDeBloqued;
t_queue *colaDeMensajes;
char* pokemonRecibido;

/* Funciones */
void inicializarVariablesGlobales();
void array_iterate_element(char** strings, void (*closure)(char*,t_list*),t_list *lista);
void enlistar(char *elemento,t_list *lista);
void obtenerDeConfig(char *clave,t_list *lista);
void gestionarEntrenador(t_entrenador *entrenador);
void crearHiloEntrenador(t_entrenador* entrenador);
t_entrenador* armarEntrenador(int id,char *posicionesEntrenador,char *objetivosEntrenador,char *pokemonesEntrenador);
void generarEntrenadores();
e_algoritmo obtenerAlgoritmoPlanificador();
void atenderBroker(int *socketBroker);
void crearHiloParaAtenderBroker(int *socketBroker);
void suscribirseALasColas(int socketA,int socketL,int socketC);
t_mensaje* parsearMensaje(char* mensaje);
t_mensaje* deserializar(void* paquete);
void gestionarMensajes(char* ip, char* puerto);
void liberarMemoria();
bool elementoEstaEnLista(t_list *lista, char *elemento);
void setearObjetivosDeTeam(t_team *team);
void enviarGetSegunObjetivo(char *ip, char *puerto);
float calcularDistancia(int posX1, int posY1,int posX2,int posY2);
t_dist *setearDistanciaEntrenadores(int id,int posX,int posY);
bool estaEnEspera(void *entrenador);
bool esUnObjetivo(void *objetivo);
bool distanciaMasCorta(void *entrenador1,void *entrenador2);
t_entrenador* entrenadorMasCercano(int posX,int posY);
void planificar(char* pokemon, int posicionX, int posicionY);

#endif /* TEAM_H_ */
