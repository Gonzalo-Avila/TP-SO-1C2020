#include "Game-Boy.h"
//#include "Team.h"

void inicializarVariablesGlobales(){
	config = config_create("gameboy.config");
	logger = log_create("gameboy_logs","GameBoy",1,LOG_LEVEL_TRACE);
}

int conectarseADestino(proceso destino){

	 char * ipDestino = malloc(16);
	 char * puertoDestino = malloc(6);
	 int socketDestino;

     switch(destino){
         case SUSCRIPTOR:
		 case BROKER:{
			 ipDestino = config_get_string_value(config,"IP_BROKER");
			 puertoDestino = config_get_string_value(config,"PUERTO_BROKER");
			 break;
		 }
		 case TEAM:{
			 ipDestino = config_get_string_value(config,"IP_TEAM");
			 puertoDestino = config_get_string_value(config,"PUERTO_TEAM");
			 break;
		 }
		 case GAMECARD:{
			 ipDestino = config_get_string_value(config,"IP_TEAM");
			 puertoDestino = config_get_string_value(config,"PUERTO_TEAM");
			 break;
		 }
		 default:{
			 log_error(logger,"Error obteniendo el destino del mensaje");
			 return -1;
		 }
     }
     socketDestino = crearConexionCliente(ipDestino,puertoDestino);
     free(ipDestino);
     free(puertoDestino);
     return socketDestino;
}
cola definirTipoMensaje(char * argumento){
     if(strcmp("NEW_POKEMON",argumento)==0){
    	 return NEW;
     }
     if(strcmp("APPEARED_POKEMON",argumento)==0){
         return APPEARED;
     }
     if(strcmp("CATCH_POKEMON",argumento)==0){
         return CATCH;
     }
     if(strcmp("CAUGHT_POKEMON",argumento)==0){
         return CAUGHT;
     }
     if(strcmp("GET_POKEMON",argumento)==0){
       	 return GET;
     }
     if(strcmp("LOCALIZED_POKEMON",argumento)==0){
       	 return LOCALIZED;
     }
     log_error(logger,"[ERROR]");
     log_error(logger,"No se pudo determinar el tipo de cola o suscripción");
     return -1;
}

proceso definirDestino(char * argumento){
     if(strcmp("TEAM",argumento)==0){
         return TEAM;
     }
     if(strcmp("GAMECARD",argumento)==0){
          return GAMECARD;
     }
     if(strcmp("BROKER",argumento)==0){
    	 return BROKER;
     }
     if(strcmp("SUSCRIPTOR",argumento)==0){
    	 return SUSCRIPTOR;
     }
     log_error(logger,"[ERROR]");
     log_error(logger,"No se pudo determinar el proceso destino");
     return -1;

}



int main(int argc, char** argv){

	//Se setean todos los datos
	log_info(logger,"Se ha iniciado el cliente gameboy\n");
	inicializarVariablesGlobales();

	proceso destino = definirDestino(argv[0]);
    cola tipoMensaje = definirTipoMensaje(argv[1]);
    int socketDestino = conectarseADestino(destino);
    if(destino==SUSCRIPTOR){
       suscribirseACola(socketDestino,tipoMensaje);
       while(sleep(atoi(argv[3]))!=0){
    	  log_info(logger,"Paso un segundo");
    	  //Recibir mensaje
    	  //Imprimir mensaje
       }
    }
    else
    {
    	switch(tipoMensaje){
    		case NEW:{
    			break;
    		}
    		case APPEARED:{
    			break;
    		}
    		case CATCH:{
    			break;
    		}
    		case CAUGHT:{
    			break;
    		}
    		case GET:{
    			break;
    		}
    		case LOCALIZED:{
    			break;
    		}

    	}
    }






    log_info(logger,"Finalizó la conexión con el servidor\n");
    log_info(logger,"El proceso team finalizó su ejecución\n");

    log_destroy(logger);
    config_destroy(config);
    return 0;


}
