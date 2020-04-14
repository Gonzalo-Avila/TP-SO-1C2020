#include "Broker.h"


/* Recibe el código de suscripción desde el socket a suscribirse, eligiendo de esta manera la cola y agregando el socket
 * a la lista de suscriptores de la misma.
 */
void atenderSuscripcion(int socketSuscriptor){

      int codSuscripcion;
      recv(socketSuscriptor,&codSuscripcion,sizeof(int),MSG_WAITALL);
      switch(codSuscripcion)
      {
           case NEW:
           {
          	 //Suscribir a NEW_POKEMON
             list_add(suscriptoresNEW,&socketSuscriptor);
             log_info(logger,"Hay un nuevo suscriptor en la cola NEW_POKEMON. Número de socket suscriptor: %d", socketSuscriptor);
             break;
           }
           case APPEARED:
           {
        	 //Suscribir a APPEARED_POKEMON
        	 list_add(suscriptoresAPP,&socketSuscriptor);
        	 log_info(logger,"Hay un nuevo suscriptor en la cola APPEARED_POKEMON. Número de socket suscriptor: %d", socketSuscriptor);
        	 break;
           }
           case CATCH:
           {
        	 //Suscribir a CATCH_POKEMON
        	 list_add(suscriptoresCAT,&socketSuscriptor);
        	 log_info(logger,"Hay un nuevo suscriptor en la cola CATCH_POKEMON. Número de socket suscriptor: %d", socketSuscriptor);
        	 break;
           }
           case CAUGHT:
           {
        	 //Suscribir a CAUGHT_POKEMON
        	 list_add(suscriptoresCAU,&socketSuscriptor);
        	 log_info(logger,"Hay un nuevo suscriptor en la cola CAUGHT_POKEMON. Número de socket suscriptor: %d", socketSuscriptor);
        	 break;
           }
           case GET:
           {
        	 //Suscribir a GET_POKEMON
        	 list_add(suscriptoresGET,&socketSuscriptor);
        	 log_info(logger,"Hay un nuevo suscriptor en la cola GET_POKEMON. Número de socket suscriptor: %d", socketSuscriptor);
        	 break;
           }
           case LOCALIZED:
           {
        	 //Suscribir a LOCALIZED_POKEMON
        	 list_add(suscriptoresLOC,&socketSuscriptor);
        	 log_info(logger,"Hay un nuevo suscriptor en la cola LOCALIZED_POKEMON. Número de socket suscriptor: %d", socketSuscriptor);
        	 break;
           }
           default:
           {
        	 log_error(logger, "Intento fallido de suscripción a una cola de mensajes");
        	 break;
           }
	   }
}

/* Guarda el mensaje en memoria cache
 * 1) Busca ponerlo en un espacio libre.
 * 2) Si no puede, compacta la memoria y vuelve a probar.
 * 3) Si no puede, borra el mensaje mas viejo.
 */
void cachearMensaje(void * mensaje){


}

/* Espera mensajes de una conexión ya establecida. Según el código de operación recibido, delega tareas a distintos modulos.
 *
 */
void esperarMensajes(int socketCliente){
	int codOperacion;
	while(1){

      recv(socketCliente,&codOperacion,sizeof(int),MSG_WAITALL);
      switch(codOperacion)
      {
        case SUSCRIPCION:
        {
            atenderSuscripcion(socketCliente);
            break;
        }
        case MENSAJE:
        {
        	/* En este punto habria que:
        	 * - Determinar a que cola va a ir ese mensaje, y agregarlo.
        	 * - Reenviar el mensaje a todos los suscriptores de dicha cola (¿o eso se hace en otro proceso asincronico?).
        	 * - Guardar el mensaje en la caché.
             *
        	 */

            break;
        }
        case FINALIZAR:
        {
        	/* Finalizaría la conexión con el broker de forma ordenada.
        	 * No creo que tenga mucho sentido en el TP, seria para hacer pruebas.
        	 */
        	break;
        }
        default:
        {
            log_error(logger,"El mensaje recibido está dañado");
        }
	  }
	}
}


/* Espera nuevas conexiones en el socket de escucha. Al establecerse una nueva, envía esa conexión a un nuevo hilo para que
 * sea gestionada y vuelve a esperar nuevas conexiones.
 */
void atenderConexiones(int socketEscucha){
	while(1){
		int * socketCliente = esperarCliente(socketEscucha, 5);
		log_info(logger,"Se ha conectado un cliente. Número de socket cliente: %d", *socketCliente);

        /* Esto me habia traido problemas antes, ¿andará asi?
         * Sino habria que crear una lista de hilos e ir agregando/quitando
         */
        pthread_t nuevoHilo;
		pthread_create(&nuevoHilo, NULL, (void*)esperarMensajes,*socketCliente);
		pthread_detach(nuevoHilo);


	}
}

void inicializarColasYListas(){

	  NEW_POKEMON=queue_create();
	  APPEARED_POKEMON=queue_create();
	  CATCH_POKEMON=queue_create();
	  CAUGHT_POKEMON=queue_create();
	  GET_POKEMON=queue_create();
	  LOCALIZED_POKEMON=queue_create();

	  suscriptoresNEW=list_create();;
	  suscriptoresAPP=list_create();;
	  suscriptoresGET=list_create();;
	  suscriptoresLOC=list_create();;
	  suscriptoresCAT=list_create();;
	  suscriptoresCAU=list_create();;
}

void inicializarVariablesGlobales(){
	config = config_create("broker.config");
	logger = log_create("broker_logs","Broker",1,LOG_LEVEL_TRACE);
	cacheBroker = malloc(config_get_int_value(config,"CACHESIZE"));

	inicializarColasYListas();
}

int main(){

	// 6 colas de mensajes - 3 pares de colas relacionadas
	//
	inicializarVariablesGlobales();

    char * puertoEscucha = malloc(strlen(config_get_string_value(config,"PUERTO"))+1);
    puertoEscucha = config_get_string_value(config,"PUERTO");

	log_info(logger,"Se ha iniciado el servidor broker\n");

	int socketEscucha = crearConexionServer(puertoEscucha);
	log_info(logger,"El servidor está configurado y a la espera de un cliente. Número de socket servidor: %d", socketEscucha);


    atenderConexiones(socketEscucha);


	/*while(1){
		tPaquete *paquete = recibirMensaje(*socketCliente);

		log_info(logger,"Mensaje recibido del socket %d\nCódigo de operación recibido: %d\nBuffer size recibido: %d\nPayload recibido: %s\n",
				*socketCliente,paquete->codOperacion,paquete->buffer->size,paquete->buffer->stream);

		if(paquete->codOperacion==FINALIZAR){
			free(paquete->buffer->stream);
			free(paquete->buffer);
		    free(paquete);
		    break;
		}

		free(paquete->buffer->stream);
        free(paquete->buffer);
		free(paquete);
	}



	close(socketEscucha);
	close(*socketCliente);
	free(socketCliente);

	log_info(logger,"El cliente se ha desconectado\n");
    log_info(logger,"El proceso broker finalizó su ejecución\n");*/

	return 0;


}
