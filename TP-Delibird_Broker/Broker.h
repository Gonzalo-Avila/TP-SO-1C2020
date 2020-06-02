/*
 * Broker.h
 *
 *  Created on: 10 abr. 2020
 *      Author: utnso
 */

#ifndef BROKER_H_
#define BROKER_H_

#include <Utils.h>
#include <time.h>
#include <signal.h>

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

//Variables de Cache
//--------------------------------
t_list * registrosDeCache;
t_list * registrosDeParticiones;
void * cacheBroker;
int CACHESIZE;
int minimoTamanioParticion;

typedef enum{
	LIBRE = 0,
	OCUPADO = 1
}estadoParticion;

typedef struct{
	int nroParticion;
	int posInicialLogica;
	void* posInicialFisica;
	int tamanioParticion;
	estadoParticion estado;
	int idMensaje;
	time_t tiempoArribo;
	time_t tiempoUltimoUso;
}registroParticion;

typedef struct {
  uint32_t idMensaje;
  uint32_t idCorrelativo;
  cola colaMensaje;
  uint32_t sizeMensaje;
  t_list * procesosALosQueSeEnvio;
  t_list * procesosQueConfirmaronRecepcion;
  void * posicionEnMemoria;
} registroCache;

typedef enum {
PARTICIONES_DINAMICAS = 0,
BUDDY_SYSTEM = 1
}t_algoritmoMemoria;
typedef enum{
FIFO = 0,
LRU = 1
}t_algoritmoReemplazo;
typedef enum{
FIRST_FIT = 0,
BEST_FIT = 1
}t_algoritmoParticionLibre;

t_algoritmoMemoria algoritmoMemoria;
t_algoritmoReemplazo algoritmoReemplazo;
t_algoritmoParticionLibre algoritmoParticionLibre;
//--------------------------------


uint32_t globalIDMensaje;
uint32_t globalIDProceso;

sem_t mutexColas;
sem_t habilitarEnvio;



typedef struct {
	int socketCliente;
	uint32_t clientID;
} suscriptor;



//#include "Broker_Cache.h"
void inicializarCache();
int chequearSiAlcanza(int sizeMensaje, void * posicionActual, int memoriaRecorrida);
void * buscarEspacio(int sizeMensaje, void *posicionInicial);
int tamanioDelMensaje(int offset, int *cacheExcedida);
void * buscarProximoMensaje(int offset, int * tamanio, int *cacheExcedida);
void compactarMemoria();
void eliminarMensaje();
void * usarBestFit(estructuraMensaje mensaje);
void * usarFirstFit(estructuraMensaje mensaje);
void * cachearConBuddySystem(estructuraMensaje mensaje);
void * cachearConParticionesDinamicas(estructuraMensaje mensaje);
void cachearMensaje(estructuraMensaje estMensaje);
void enviarMensajesCacheados(suscriptor * nuevoSuscriptor, cola codSuscripcion);
void dumpCache();
bool estaOcupado(void* regParticion);
registroParticion * vaciarParticion();
bool hayEspacioLibrePara(int sizeMensaje);




//#include "Broker_Recepcion.h"
void empezarAAtenderCliente(int socketEscucha);
void atenderConexiones(int *socketEscucha);
void esperarMensajes(int *socketCliente);
bool yaExisteSuscriptor(uint32_t clientID, cola codSuscripcion);
void atenderSuscripcion(int *socketSuscriptor);
void atenderMensaje(int socketEmisor, cola tipoCola);
void imprimirEstructuraDeDatos(estructuraMensaje mensaje);
estructuraMensaje * generarNodo(estructuraMensaje mensaje);
int agregarMensajeACola(int socketEmisor, cola tipoCola, int idCorrelativo);
void crearRegistroCache(estructuraMensaje mensaje, void* posInicialMemoria);
bool compararPorMenorTamanio(void * particion1, void * particion2);
void reasignarNumerosDeParticion(t_list * listaAReasignar);
void aniadirNuevoRegistroALista(t_list * listaDeRegistros, registroParticion * registroAnterior, int sizeMensajeRecibido);


//#include "Broker_Envio.h"
void enviarEstructuraMensajeASuscriptor(void* estMensaje);
bool esMensajeNuevo(void* mensaje);
void atenderColas();
void agregarAListaDeConfirmados(uint32_t idMsg, uint32_t idProceso);
void agregarAListaDeEnviados(uint32_t idMsg, uint32_t idProceso);
void destructorNodos(void * nodo);

//#include "Broker_Aux.h"
void inicializarVariablesGlobales();
void destruirVariablesGlobales();
void liberarSocket(int* socket);
void inicializarColasYListas();
int getSocketEscuchaBroker();
void* deserializarPayload(int socketSuscriptor);
t_list * getColaByNum(int nro);
t_list* getListaSuscriptoresByNum(opCode nro);
uint32_t getIDMensaje();
uint32_t getIDProceso();
void desuscribir(uint32_t clientID, cola colaSuscripcion);
void eliminarSuscriptor(t_list* listaSuscriptores, uint32_t clientID);
int getSocketActualDelSuscriptor(uint32_t clientID, cola colaSuscripcion);
suscriptor * buscarSuscriptor(uint32_t clientID, cola codSuscripcion);
int XOR(int a, int b);


#endif /* BROKER_H_ */
