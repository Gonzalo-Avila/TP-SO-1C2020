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
	pthread_cond_t *cond;
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
	int id;
}t_listaHilos;

typedef struct{
	float dist;
	int id;
}t_dist;

typedef struct{
	char *pokemon;
	t_list *x;
	t_list *y;
	t_list *cantidades;
}t_posicionEnMapa;

t_team *team;
t_list *listaHilos;
t_list *listaDeReady;
t_list *listaDeBloqued;
char* pokemonRecibido;
char* ipServidor;
char* puertoServidor;

t_list *listaCondsEntrenadores;
t_list *listaPosicionesInternas;

pthread_mutex_t mutexHilosEntrenadores;
sem_t *mutexMensajes;
sem_t *mutexEntrenadores;
sem_t *semPlanif;

/* Funciones */
void inicializarVariablesGlobales();
uint32_t obtenerIdDelProceso(char* ip, char* puerto);
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
void suscribirseALasColas(int socketA,int socketL,int socketC, uint32_t idProceso);
t_mensaje* parsearMensaje(char* mensaje);
t_mensaje* deserializar(void* paquete);
void gestionarMensajes(char* ip, char* puerto);
bool menorDist(void *dist1,void *dist2);
bool hayaAlgunEntrenadorActivo();
void liberarMemoria();
t_list *obtenerEntrenadoresReady();
bool elementoEstaEnLista(t_list *lista, char *elemento);
void setearObjetivosDeTeam();
void enviarGetSegunObjetivo(char *ip, char *puerto);
void enviarGetDePokemon(char *ip, char *puerto, char *pokemon);
float calcularDistancia(int posX1, int posY1,int posX2,int posY2);
t_dist *setearDistanciaEntrenadores(int id,int posX,int posY);
bool estaEnEspera(t_entrenador *entrenador);
bool esUnObjetivo(void *objetivo);
bool distanciaMasCorta(void *entrenador1,void *entrenador2);
t_entrenador* entrenadorMasCercanoEnEspera(int posX,int posY);
void planificarFifo();
void planificador();
void setearCondsEntrenadores();
void ponerEnReadyAlMasCercano(int x, int y);

#endif /* TEAM_H_ */
