/*
 * Broker.h
 *
 *  Created on: 10 abr. 2020
 *      Author: utnso
 */

#ifndef BROKER_H_
#define BROKER_H_

#include <Utils.h>
#include <semaphore.h>


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
long globalID=1;

sem_t mutexColas;
sem_t habilitarEnvio;


//t_list* getListaSuscriptoresByNum(int nro);
int chequearSiAlcanza(int sizeMensaje, void * posicionActual, int memoriaRecorrida);
void * buscarEspacio(int sizeMensaje, void *posicionInicial);
int tamanioDelMensaje(int offset, int *cacheExcedida);
void * buscarProximoMensaje(int offset, int * tamanio, int *cacheExcedida);
void compactarMemoria();
void eliminarMensaje();
void cachearMensaje(void * mensaje, int sizeMensaje);
void enviarMensajesCacheados(int socketSuscriptor, int codSuscripcion);
void* deserializarPayload(int socketSuscriptor);
char* getCodeStringByNum(int nro);
t_list * getColaByNum(int nro);
t_list* getListaSuscriptoresByNum(opCode nro);
void *getEnviarAColaByNum(int num);
long getID();
void imprimirEstructuraDeDatos(estructuraMensaje mensaje);
int agregarMensajeACola(int socketEmisor, cola tipoCola, int idCorrelativo);
void atenderMensaje(int socketEmisor, cola tipoCola);
void atenderSuscripcion(int *socketSuscriptor);
void esperarMensajes(int *socketCliente);
void atenderConexiones(int *socketEscucha);
/*estructuraMensaje nodoAEstructura(estructuraMensaje nodoMsj);
void enviarMensajeASuscriptorNEW(estructuraMensaje nodoMsj);
void enviarMensajeASuscriptorAPP(estructuraMensaje nodoMsj);
void enviarMensajeASuscriptorCAT(estructuraMensaje nodoMsj);
void enviarMensajeASuscriptorCAU(estructuraMensaje nodoMsj);
void enviarMensajeASuscriptorGET(estructuraMensaje nodoMsj);
void enviarMensajeASuscriptorLOC(estructuraMensaje nodoMsj);*/
void enviarEstructuraMensajeASuscriptor(void* estMensaje);
bool esMensajeNuevo(void* mensaje);
void atenderColas();
void inicializarColasYListas();
void inicializarCache();
void inicializarVariablesGlobales();
void destruirVariablesGlobales();
void liberarSocket(int* socket);
int getSocketEscuchaBroker();
void empezarAAtenderCliente(int socketEscucha);
void empezarAAtenderCliente();

#endif /* BROKER_H_ */
