#include "Team.h"


int main(){

	t_config * config = config_create("team.config");
	t_log * logger = log_create("team_logs","Team",1,LOG_LEVEL_TRACE);
	char * ipServidor = malloc(strlen(config_get_string_value(config,"IP"))+1);
	ipServidor = config_get_string_value(config,"IP");
	char * puertoServidor = malloc(strlen(config_get_string_value(config,"PUERTO"))+1);
	puertoServidor = config_get_string_value(config,"PUERTO");
    log_info(logger,"Se ha iniciado el cliente team\n");

	int socketServidor = crearConexionCliente(ipServidor,puertoServidor);
	log_info(logger,"Se ha establecido conexión con el servidor\nIP: %s\nPuerto: %s\nNúmero de socket: %d",
			config_get_string_value(config,"IP"),config_get_string_value(config,"PUERTO"));

    while(1){
    	char * mensaje = malloc(MAXSIZE);
      	printf("Ingrese mensaje: ");
      	scanf("%s",mensaje);
      	enviarMensaje(mensaje,socketServidor);
        if(strcmp(mensaje,"exit")==0)
        	break;
    }
    close(socketServidor);
    log_info(logger,"Finalizó la conexión con el servidor\n");
    log_info(logger,"El proceso team finalizó su ejecución\n");
    return 0;


}
