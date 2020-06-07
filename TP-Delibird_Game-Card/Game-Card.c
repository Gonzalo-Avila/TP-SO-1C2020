#include "Game-Card.h"


void inicializarVariablesGlobales(){
	config = config_create("gamecard.config");
	logger = log_create("gamecard_logs","GameCard",1,LOG_LEVEL_TRACE);
}

void cerrarConexion(){
    free(ipServidor);
    free(puertoServidor);
    log_info(logger,"Finalizó la conexión con el servidor\n");
}

void destruirVariablesGlobales(){
    log_destroy(logger);
    config_destroy(config);
}

void esperarMensajesDeBrokerEnCola(int * socketSuscripcion){
	mensajeRecibido * mensaje = recibirMensajeDeBroker(*socketSuscripcion);
    switch(mensaje->colaEmisora)
    {
    	case NEW:
    	{
    		//Procesar mensaje NEW
    		log_debug(logger,"Llegó un mensaje de la cola NEW");
    		break;
    	}
    	case GET:
    	{
    	   	//Procesar mensaje NEW
    		log_debug(logger,"Llegó un mensaje de la cola GET");
    	   	break;
    	    	}
    	case CATCH:
    	{
    	   	//Procesar mensaje NEW
    		log_debug(logger,"Llegó un mensaje de la cola CATCH");
    	   	break;
    	}
    	default:
    	{
    		log_error(logger,"Mensaje recibido de una cola no correspondiente");
    		break;
    	}
    }
}

int crearConexionBroker(){
	char * ipServidor= malloc(strlen(config_get_string_value(config,"IP_BROKER"))+1);
	char * puertoServidor = malloc(strlen(config_get_string_value(config,"PUERTO_BROKER"))+1);
	return  obtenerIdDelProceso(ipServidor, puertoServidor);
}

int suscribirA(cola colaMensaje){

	int socketConexion;
	ipServidor= malloc(strlen(config_get_string_value(config,"IP_BROKER"))+1);
	puertoServidor = malloc(strlen(config_get_string_value(config,"PUERTO_BROKER"))+1);

	ipServidor = config_get_string_value(config,"IP_BROKER");
	puertoServidor = config_get_string_value(config,"PUERTO_BROKER");


	socketConexion = crearConexionCliente(ipServidor,puertoServidor);
	suscribirseACola(socketConexion, colaMensaje, idProceso);
	return socketConexion;

}

void realizarFunciones(){
	int a=0;
	scanf("%d", &a);
}

void crearSuscripcionesBroker(){

    pthread_t hiloEsperaNEW, hiloEsperaGET, hiloEsperaCATCH;

    int socketSuscripcionNEW = suscribirA(NEW);
	int socketSuscripcionGET = suscribirA(GET);
	int socketSuscipcionCATCH = suscribirA(CATCH);

    pthread_create(&hiloEsperaNEW, 		NULL, (void*) esperarMensajesDeBrokerEnCola, &socketSuscripcionNEW);
    pthread_create(&hiloEsperaGET, 		NULL, (void*) esperarMensajesDeBrokerEnCola, &socketSuscripcionGET);
    pthread_create(&hiloEsperaCATCH, 	NULL, (void*) esperarMensajesDeBrokerEnCola, &socketSuscipcionCATCH);

    pthread_detach(hiloEsperaNEW);
    pthread_detach(hiloEsperaGET);
    pthread_detach(hiloEsperaCATCH);
}

int main(){
    //Se setean todos los datos

	log_info(logger,"Se ha iniciado el cliente GameCard\n");

	inicializarVariablesGlobales();

	idProceso = crearConexionBroker();

	crearSuscripcionesBroker();

    realizarFunciones();

    cerrarConexion();

    destruirVariablesGlobales();

    //Procedimiento auxiliar para que no rompa el server en las pruebas
    /*int codigoOP = FINALIZAR;
    send(socketBroker,(void*)&codigoOP,sizeof(opCode),0);
    close(socketBroker);*/

    log_info(logger,"El proceso gamecard finalizó su ejecución\n");

    return 0;
}
