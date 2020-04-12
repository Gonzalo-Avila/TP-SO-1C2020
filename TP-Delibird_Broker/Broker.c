#include "Broker.h"

int main(){

	// 6 colas de mensajes - 3 pares de colas relacionadas
	//
	//inicializarColas();
    t_config * config = config_create("broker.config");
    t_log * logger = log_create("broker_logs","Broker",1,LOG_LEVEL_TRACE);
    char * puertoEscucha = malloc(strlen(config_get_string_value(config,"PUERTO"))+1);
    puertoEscucha = config_get_string_value(config,"PUERTO");

	log_info(logger,"Se ha iniciado el servidor broker\n");

	int socketEscucha = crearConexionServer(puertoEscucha);
	log_info(logger,"El servidor está configurado y a la espera de un cliente. Número de socket servidor: %d", socketEscucha);

	int *socketCliente = esperarCliente(socketEscucha, 5);
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

	log_info(logger,"El cliente se ha desconectado\n");
    log_info(logger,"El proceso broker finalizó su ejecución\n");

	return 0;


}
