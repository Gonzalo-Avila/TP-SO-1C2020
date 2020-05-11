/*
 * Broker.h
 *
 *  Created on: 10 abr. 2020
 *      Author: utnso
 */

#ifndef BROKER_H_
#define BROKER_H_

#include <Utils.h>
#include <string.h>
#include <stdbool.h>

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

uint32_t globalIDMensaje;
uint32_t globalIDProceso;

sem_t mutexColas;
sem_t habilitarEnvio;

//#include "Broker_Cache.h"
void inicializarCache();
int chequearSiAlcanza(int sizeMensaje, void * posicionActual, int memoriaRecorrida);
void * buscarEspacio(int sizeMensaje, void *posicionInicial);
int tamanioDelMensaje(int offset, int *cacheExcedida);
void * buscarProximoMensaje(int offset, int * tamanio, int *cacheExcedida);
void compactarMemoria();
void eliminarMensaje();
void cachearConBuddySystem(void * mensaje, int sizeMensaje);
void usarBestFit();
void usarFirstFit(void * mensaje, int sizeMensaje);
void cachearConParticionesDinamicas(void * mensaje, int sizeMensaje);
void cachearMensaje(void * mensaje, int sizeMensaje);
void enviarMensajesCacheados(int socketSuscriptor, int codSuscripcion);

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


//#include "Broker_Envio.h"
void enviarEstructuraMensajeASuscriptor(void* estMensaje);
bool esMensajeNuevo(void* mensaje);
void atenderColas();

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


#endif /* BROKER_H_ */
