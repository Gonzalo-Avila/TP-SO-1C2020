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

/* Chequea la disponibilidad de tantas posiciones contiguas como sea el tamaño del mensaje. Si en el proceso encuentra alguna
 * ocupada retorna 0 (no alcanza el espacio), si termina sin encontrar ninguna ocupada retorna 1 (espacio suficiente).
 */
int chequearSiAlcanza(int sizeMensaje,void * posicionActual, int memoriaRecorrida){
   for(int offset=1; offset<sizeMensaje; offset++)
   {
       if (memoriaRecorrida==CACHESIZE || *(char *)(posicionActual+offset)=='f' ) //LAZY EVALUATION POWA
         return 0;
       memoriaRecorrida++;
   }
   return 1;

}
/* Busca secuencialmente algun byte marcado como libre, cuando lo encuentra llama a otra funcion para ver si alcanza el
 * espacio para guardar el mensaje
 *
 */
void * buscarEspacio(int sizeMensaje, void *posicionInicial){
    int espacioSuficiente = 0;
    int offset = 0;
    while(!espacioSuficiente){
    	if (offset==CACHESIZE)
    	    return NULL;

        if(*(char *)(posicionInicial+offset)=='f'){
          espacioSuficiente=chequearSiAlcanza(sizeMensaje,posicionInicial+offset,offset);
        }
        offset++;

    }
    return posicionInicial+offset;
}

/* Partiendo desde una posición ocupada, cuenta todas las posiciónes contiguas llenas para determinar el tamaño del mensaje
 *
 */
int tamanioDelMensaje(int offset, int *cacheExcedida){
	int tamanio=0;
	while(*(char *)(cacheBroker+offset)!='f'){
		tamanio++;
		offset++;
		if(offset==CACHESIZE){
		   	 *cacheExcedida=1;
		   	 break;
	    }
	}
	return tamanio;
}

/* Partiendo desde una posición vacia, busca secuencialmente hasta encontrar una posición ocupada
 *
 */
void * buscarProximoMensaje(int offset, int * tamanio, int *cacheExcedida){
     while(*(char *)(cacheBroker+offset)!='f'){
    	 offset++;
    	 if(offset==CACHESIZE){
    		 *cacheExcedida=1;
    		 return NULL;
    	 }
     }
     *tamanio=tamanioDelMensaje(offset,cacheExcedida);

     return cacheBroker+offset;

}

/* Libera los "huecos" de la memoria cache, haciendo que todos los mensajes esten pegados uno detras del otro.
 *
 */

void compactarMemoria(){
	int tamanio;
	int cacheExcedida=0;
	void * posicion;
	for(int offset=0 ; offset<CACHESIZE; offset++){
		if(*(char *)(cacheBroker+offset)=='f'){
			posicion=buscarProximoMensaje(offset,&tamanio,&cacheExcedida);
			if(cacheExcedida==0)
			{
				memcpy(cacheBroker+offset,posicion,tamanio);
				memset(posicion,'f',tamanio);
			}
			else
			{
				break;
			}
		}
	}
}

//A desarrollar
void eliminarMensaje(){
}

/* Guarda el mensaje en memoria cache mediante algoritmo first fit
 * 1) Busca ponerlo en un espacio libre.
 * 2) Si no puede, compacta la memoria y vuelve a probar.
 * 3) Si no puede, borra el mensaje mas viejo y vuelve a probar.
 * 4) Si no puede, repite 2 y 3 hasta que lo mete.
 *
 * - El algoritmo deberia hacer una n cantidad de busquedas de espacio antes de compactar (??????????)
 * - Hay que validar el caso extremo en el que el mensaje que se quiere guardar no entre en toda cache, en esta función o en otra
 * - Hay que ver como poner las distintos criterios de victima
 */
void cachearMensaje(void * mensaje, int sizeMensaje){

       void * particion;
       particion = buscarEspacio(sizeMensaje,cacheBroker);
       while(particion==NULL){
           compactarMemoria();
           particion = buscarEspacio(sizeMensaje,cacheBroker);
           if(particion==NULL)
           {
              eliminarMensaje();
              particion = buscarEspacio(sizeMensaje,cacheBroker);
           }
       }
       memcpy(particion,mensaje,sizeMensaje);
}

/* Espera mensajes de una conexión ya establecida. Según el código de operación recibido, delega tareas a distintos modulos.
 *
 */
void esperarMensajes(int socketCliente){
	int codOperacion;
    int desconectar=0;
	while(desconectar==0){

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
             * - ...
        	 */

            break;
        }
        case FINALIZAR:
        {
        	/* Finaliza la conexión con el broker de forma ordenada.
        	 * No creo que tenga mucho sentido en el TP, seria para hacer pruebas.
        	 */
        	desconectar=1;
        	break;
        }
        default:
        {
            log_error(logger,"El mensaje recibido está dañado");
            break;
        }
	  }
	}
    log_info(logger, "El cliente con socket %d se ha desconectado",socketCliente);
}


/* Espera nuevas conexiones en el socket de escucha. Al establecerse una nueva, envía esa conexión a un nuevo hilo para que
 * sea gestionada y vuelve a esperar nuevas conexiones.
 */
void atenderConexiones(int socketEscucha){
	char * backlog_server = malloc(strlen(config_get_string_value(config,"BACKLOG_SERVER"))+1);
	backlog_server = config_get_string_value(config,"BACKLOG_SERVER");
	atenderConexionEn(socketEscucha, backlog_server);
	while(1){
		int socketCliente = esperarCliente(socketEscucha);
		log_info(logger,"Se ha conectado un cliente. Número de socket cliente: %d", socketCliente);

        /* Esto me habia traido problemas antes, ¿andará asi?
         * Sino habria que crear una lista de hilos e ir agregando/quitando
         */
        pthread_t nuevoHilo;
		pthread_create(&nuevoHilo, NULL, (void*)esperarMensajes,socketCliente); //No entiendo el warning, si le paso un puntero no anda
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

/* La cache tiene que ser un espacio fijo, reservado al momento de ejecutar el broker. Tiene que ser un espacio tipo void
 * porque no se sabe que se va a guardar ahi, pero tambien tiene que estar inicializado para poder diferenciar que espacios
 * estan vacios. Hay que averiguar como hacer esto bien, puse un caracter para llenar.
 */
void inicializarCache(){
	CACHESIZE=config_get_int_value(config,"TAMANO_MEMORIA");
	cacheBroker = malloc(CACHESIZE);

    memset(cacheBroker,'f',CACHESIZE);
}

void inicializarVariablesGlobales(){
	config = config_create("broker.config");
	logger = log_create("broker_logs","Broker",1, LOG_LEVEL_TRACE);

	inicializarColasYListas();
	inicializarCache();
}

int main(){


	inicializarVariablesGlobales();

	char * ipEscucha = malloc(strlen(config_get_string_value(config,"IP_BROKER"))+1);
	ipEscucha = config_get_string_value(config,"IP_BROKER");

    char * puertoEscucha = malloc(strlen(config_get_string_value(config,"PUERTO_BROKER"))+1);
    puertoEscucha = config_get_string_value(config,"PUERTO_BROKER");

	log_info(logger,"Se ha iniciado el servidor broker\n");

	int socketEscucha = crearConexionServer(ipEscucha, puertoEscucha);
	log_info(logger,"El servidor está configurado y a la espera de un cliente. Número de socket servidor: %d", socketEscucha);

	//Un hilo iria a atender conexiones
    atenderConexiones(socketEscucha);

    //Otros hilos seguirian aca
    free(puertoEscucha);
    log_destroy(logger);
    config_destroy(config);

	return 0;


}
