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
	char* pokemon;
	t_posicion pos;
}t_objetivoActual;

typedef struct{
	int id;
	t_posicion pos;
	t_list *objetivos;
	t_list *pokemones;
	e_estado estado;
	bool suspendido;
	t_objetivoActual pokemonAAtrapar;
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

typedef struct{
	uint32_t idCorrelativo;
	t_entrenador* entrenadorConCatch;
}t_catchEnEspera;

t_team *team;
t_list *listaHilos;
t_list *listaDeReady;
t_list *listaDeBloqued;
t_list *idsDeCatch;

char* ipServidor;
char* puertoServidor;

t_list *listaCondsEntrenadores;
t_list *listaPosicionesInternas;

sem_t mutexMensajes;
sem_t mutexEntrenadores;
sem_t semPlanif;
sem_t *semEntrenadores;
sem_t *semEntrenadoresRR;
sem_t procesoEnReady;

/* Funciones */
bool menorDist(void *dist1,void *dist2);
bool noSeCumplieronLosObjetivos();
bool elementoEstaEnLista(t_list *lista, char *elemento);
bool estaEnEspera(t_entrenador *entrenador);
bool distanciaMasCorta(void *entrenador1,void *entrenador2);
bool estaEnLosObjetivos(char *pokemon);
bool validarIDCorrelativoCatch(uint32_t id);
float calcularDistancia(int posX1, int posY1,int posX2,int posY2);
int crearConexionEscuchaGameboy();
e_algoritmo obtenerAlgoritmoPlanificador();
t_catchEnEspera* buscarCatch(uint32_t idCorrelativo);
t_dist *setearDistanciaEntrenadores(int id,int posX,int posY);
t_entrenador* armarEntrenador(int id,char *posicionesEntrenador,char *objetivosEntrenador,char *pokemonesEntrenador);
t_entrenador* entrenadorMasCercanoEnEspera(int posX,int posY);
t_list *obtenerEntrenadoresReady();
t_mensaje* deserializar(void* paquete);
void inicializarVariablesGlobales();
void array_iterate_element(char** strings, void (*closure)(char*,t_list*),t_list *lista);
void enlistar(char *elemento,t_list *lista);
void obtenerDeConfig(char *clave,t_list *lista);
void gestionarEntrenador(t_entrenador *entrenador);
void gestionarEntrendorFIFO(t_entrenador *entrenador);
void gestionarEntrenadorRR(t_entrenador* entrenador);
void crearHiloEntrenador(t_entrenador* entrenador);
void crearHilosDeEntrenadores();
void generarEntrenadores();
void atenderServidor(int *socketServidor);
void crearHiloParaAtenderServidor(int *socketServidor);
void crearHilosParaAtenderBroker(int *socketBrokerApp, int *socketBrokerLoc, int *socketBrokerCau);
void suscribirseALasColas(int socketA,int socketL,int socketC, uint32_t idProceso);
void atenderGameboy(int *socketEscucha);
void esperarMensajes(int *socketCliente);
void gestionarMensajes(char* ip, char* puerto);
void liberarMemoria();
void setearObjetivosDeTeam();
void enviarGetSegunObjetivo(char *ip, char *puerto);
void enviarGetDePokemon(char *ip, char *puerto, char *pokemon);
void enviarCatchDePokemon(char *ip, char *puerto, t_entrenador* entrenador);
void planificarFifo();
void planificador();
void ponerEnReadyAlMasCercano(int x, int y, char* pokemon);
void moverXDelEntrenador(t_entrenador *entrenador);
void moverYDelEntrenador(t_entrenador *entrenador);
void inicializarSemEntrenadores();
void procesarObjetivoCumplido(t_catchEnEspera* catchProcesado, uint32_t resultado);
void borrarPokemonDeObjetivos(char* pokemonAtrapado, t_list* objetivos);
void activarHiloDe(int id);
void activarHiloDeRR(int id, int quantum);
//uint32_t obtenerIdDelProceso(char* ip, char* puerto);

#endif /* TEAM_H_ */
