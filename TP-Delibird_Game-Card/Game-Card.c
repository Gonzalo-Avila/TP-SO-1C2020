#include "Game-Card.h"


void inicializarVariablesGlobales(){
	config = config_create("gamecard.config");
	logger = log_create("gamecard_logs","GameCard",1,LOG_LEVEL_TRACE);
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

void conectarYSuscribir(int * socketSuscripcionNEW, int * socketSuscripcionGET, int * socketSuscripcionCATCH,char * ipServidor,char * puertoServidor){

	 idProceso = obtenerIdDelProceso(ipServidor, puertoServidor);
	 *socketSuscripcionNEW = crearConexionCliente(ipServidor,puertoServidor);
	 suscribirseACola(*socketSuscripcionNEW, NEW, idProceso);
	 *socketSuscripcionGET = crearConexionCliente(ipServidor,puertoServidor);
	 suscribirseACola(*socketSuscripcionGET, GET, idProceso);
	 *socketSuscripcionCATCH = crearConexionCliente(ipServidor,puertoServidor);
	 suscribirseACola(*socketSuscripcionCATCH, CATCH, idProceso);
}

void realizarFunciones(){
	int a=0;
	scanf("%d",a);
}

int main(){
    //Se setean todos los datos
	inicializarVariablesGlobales();

    pthread_t hiloEsperaNEW, hiloEsperaGET, hiloEsperaCATCH;
    int socketSuscripcionNEW, socketSuscripcionGET, socketSuscipcionCATCH;

	char * ipServidor = malloc(strlen(config_get_string_value(config,"IP_BROKER"))+1);
	ipServidor = config_get_string_value(config,"IP_BROKER");
	char * puertoServidor = malloc(strlen(config_get_string_value(config,"PUERTO_BROKER"))+1);
	puertoServidor = config_get_string_value(config,"PUERTO_BROKER");
    log_info(logger,"Se ha iniciado el cliente GameCard\n");


    conectarYSuscribir(&socketSuscripcionNEW, &socketSuscripcionGET, &socketSuscipcionCATCH,ipServidor,puertoServidor);


    pthread_create(&hiloEsperaNEW, NULL, (void*) esperarMensajesDeBrokerEnCola,
    			&socketSuscripcionNEW);
    pthread_detach(hiloEsperaNEW);

    pthread_create(&hiloEsperaGET, NULL, (void*) esperarMensajesDeBrokerEnCola,
        			&socketSuscripcionGET);
    pthread_detach(hiloEsperaGET);

    pthread_create(&hiloEsperaCATCH, NULL, (void*) esperarMensajesDeBrokerEnCola,
        			&socketSuscipcionCATCH);
        pthread_detach(hiloEsperaCATCH);


    realizarFunciones();
    free(ipServidor);
    free(puertoServidor);

    //Procedimiento auxiliar para que no rompa el server en las pruebas
    /*int codigoOP = FINALIZAR;
    send(socketBroker,(void*)&codigoOP,sizeof(opCode),0);
    close(socketBroker);*/

    log_info(logger,"Finalizó la conexión con el servidor\n");
    log_info(logger,"El proceso gamecard finalizó su ejecución\n");

    log_destroy(logger);
    config_destroy(config);

    return 0;
}
