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

typedef uint32_t t_posicion[2];

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
	float duracionRafagaAnt;
	float duracionRafagaAct;
	float estimadoRafagaAnt;
	float estimadoRafagaAct;
	bool fueDesalojado;
}t_datosSjf;

typedef struct {
	bool estaEnDeadlock;
	char *pokemonAIntercambiar;
	int idEntrenadorAIntercambiar;
}t_datosDeadlock;

typedef struct{
	int id;
	t_posicion pos;
	t_list *objetivos;
	t_list *pokemones;
	int cantidadMaxDePokes;
	e_estado estado;
	bool suspendido;
	t_objetivoActual pokemonAAtrapar;
	t_datosSjf datosSjf;
	t_datosDeadlock datosDeadlock;
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
	t_posicion pos;
}t_posicionEnMapa;

typedef struct{
	uint32_t idCorrelativo;
	t_entrenador* entrenadorConCatch;
}t_catchEnEspera;

int tiempoDeEspera;
uint32_t idDelProceso;
float alfa;
t_team *team;
t_list *listaHilos;
t_list *listaDeReady;
t_list *listaDeBloqued;
t_list *idsDeCatch;

int *socketBrokerApp;
int *socketBrokerLoc;
int *socketBrokerCau;
int *socketGameboy;

char* ipServidor;
char* puertoServidor;

t_list *listaCondsEntrenadores;
t_list *listaPosicionesInternas;

sem_t mutexMensajes;
sem_t mutexEntrenadores;
sem_t mutexAPPEARED;
sem_t mutexLOCALIZED;
sem_t mutexCAUGHT;
sem_t semPlanif;
sem_t *semEntrenadores;
sem_t *semEntrenadoresRR;
sem_t procesoEnReady;
sem_t conexionCreada;
sem_t resolviendoDeadlock;

pthread_t hiloPlanificador;

/* Funciones */
bool menorDist(void *dist1,void *dist2);
bool noSeCumplieronLosObjetivos();
bool elementoEstaEnLista(t_list *lista, char *elemento);
bool estaEnEspera(t_entrenador *entrenador);
bool distanciaMasCorta(void *entrenador1,void *entrenador2);
bool estaEnLosObjetivos(char *pokemon);
bool validarIDCorrelativoCatch(uint32_t id);
bool menorEstimacion(void* entrenador1, void* entrenador2);
bool hayNuevoEntrenadorConMenorRafaga(t_entrenador* entrenador);
float calcularDistancia(int posX1, int posY1,int posX2,int posY2);
float calcularEstimacion(t_entrenador* entrenador);
void crearConexionEscuchaGameboy(int* socket);
e_algoritmo obtenerAlgoritmoPlanificador();
t_catchEnEspera* buscarCatch(uint32_t idCorrelativo);
t_dist *setearDistanciaEntrenadores(int id,int posX,int posY);
t_entrenador* armarEntrenador(int id, char *posicionesEntrenador,char *objetivosEntrenador,
		char *pokemonesEntrenador, float estInicialEntrenador);
int entrenadorMasCercanoEnEspera(int posX,int posY);
t_entrenador* entrenadorConMenorRafaga();
t_list *obtenerEntrenadoresReady();
t_mensaje* deserializar(void* paquete);
void inicializarVariablesGlobales();
void array_iterate_element(char** strings, void (*closure)(char*,t_list*),t_list *lista);
void enlistar(char *elemento,t_list *lista);
void obtenerDeConfig(char *clave,t_list *lista);
void gestionarEntrenador(t_entrenador *entrenador);
void gestionarEntrenadorFIFO(t_entrenador *entrenador);
void gestionarEntrenadorRR(t_entrenador* entrenador);
void crearHiloEntrenador(t_entrenador* entrenador);
void crearHilosDeEntrenadores();
void generarEntrenadores();
void atenderServidor(int *socketServidor);
void crearHiloParaAtenderServidor(int *socketServidor);
//void crearHilosParaAtenderBroker(int *socketBrokerApp, int *socketBrokerLoc, int *socketBrokerCau);
void crearConexionesYSuscribirseALasColas();
void conectarGameboy();
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
void crearConexionesCliente(int* socketBrokerLoc, int* socketBrokerApp, int* socketBrokerCau);
//void borrarPokemonDeObjetivos(char* pokemonAtrapado, t_list* objetivos);
void activarHiloDe(int id);
void activarHiloDeRR(int id, int quantum);
void planificarSJFsinDesalojo();
void planificarSJFconDesalojo();
void gestionarEntrenadorSJFconDesalojo(t_entrenador* entrenador);
void gestionarEntrenadorSJFsinDesalojo(t_entrenador* entrenador);
void esperarMensajesGameboy(int* socketSuscripcion);
void crearHiloPlanificador();
void escaneoDeDeadlock();
int crearConexionClienteConReintento(char * ip, char * puerto, int tiempoDeEspera);

//uint32_t obtenerIdDelProceso(char* ip, char* puerto);

#endif /* TEAM_H_ */
