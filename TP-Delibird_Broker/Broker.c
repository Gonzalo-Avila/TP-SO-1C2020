#include "Broker.h"



void atenderSuscripcion(int socketEmisor){
    int codSuscripcion;
    recv(socketEmisor,&codSuscripcion,sizeof(int),MSG_WAITALL);
    switch(codSuscripcion)
    {
         case NEW:
         {
        	 //Suscribir a NEW_POKEMON
             break;
         }
         case APPEARED:
         {
        	 //Suscribir a APPEARED_POKEMON
        	 break;
         }
         case CATCH:
         {
        	 //Suscribir a CATCH_POKEMON
        	 break;
         }
         case CAUGHT:
         {
        	 //Suscribir a CAUGHT_POKEMON
        	 break;
         }
         case GET:
         {
        	 //Suscribir a GET_POKEMON
        	 break;
         }
         case LOCALIZED:
         {
        	 //Suscribir a LOCALIZED_POKEMON
        	 break;
         }
         default:
         {

         }
    }
}

void inicializarColas(){
	  NEW_POKEMON=queue_create();
	  APPEARED_POKEMON=queue_create();
	  CATCH_POKEMON=queue_create();
	  CAUGHT_POKEMON=queue_create();
	  GET_POKEMON=queue_create();
	  LOCALIZED_POKEMON=queue_create();
}

void inicializarVariablesGlobales(){
	config = config_create("broker.config");
	logger = log_create("broker_logs","Broker",1,LOG_LEVEL_TRACE);
	listaSocketsCliente=queue_create();
	inicializarColas();
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

	/* Aca iria un while donde el broker esperaria nuevos clientes, y por cada uno que llegue iria a un nuevo thread
	 * para atender las sucripciones, y posteriormente el envio/recepcion de mensajes. Tambien podria haber un thread
	 * nuevo para separar atencion de nuevos suscriptores y atencion de clientes ya suscriptos
	 */
	int * socketCliente = esperarCliente(socketEscucha, 5);
	log_info(logger,"Se ha conectado un cliente. Número de socket cliente: %d", *socketCliente);


	while(1){
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
    log_info(logger,"El proceso broker finalizó su ejecución\n");

	return 0;


}
