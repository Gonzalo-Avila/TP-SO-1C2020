#include "Team.h"

void inicializarVariablesGlobales(){
	config = config_create("team.config");
	logger = log_create("team_logs","Team",1,LOG_LEVEL_TRACE);
}
int main(){
	//Se setean todos los datos
	inicializarVariablesGlobales();

	char * ipServidor = malloc(strlen(config_get_string_value(config,"IP"))+1);
	ipServidor = config_get_string_value(config,"IP");
	char * puertoServidor = malloc(strlen(config_get_string_value(config,"PUERTO"))+1);
	puertoServidor = config_get_string_value(config,"PUERTO");
    log_info(logger,"Se ha iniciado el cliente team\n");

    //Se crea la conexion con el broker. Esto posteriormente debe ir con un sistema de reintentos por si el broker esta off
	int socketBrokerAPP = crearConexionCliente(ipServidor,puertoServidor);
	log_info(logger,"Se ha establecido conexión con el servidor\nIP: %s\nPuerto: %s\nNúmero de socket: %d",
			config_get_string_value(config,"IP"),config_get_string_value(config,"PUERTO"));

	/*int socketBrokerLOC = crearConexionCliente(ipServidor,puertoServidor);
		log_info(logger,"Se ha establecido conexión con el servidor\nIP: %s\nPuerto: %s\nNúmero de socket: %d",
				config_get_string_value(config,"IP"),config_get_string_value(config,"PUERTO"));

	int socketBrokerCAU = crearConexionCliente(ipServidor,puertoServidor);
		log_info(logger,"Se ha establecido conexión con el servidor\nIP: %s\nPuerto: %s\nNúmero de socket: %d",
				config_get_string_value(config,"IP"),config_get_string_value(config,"PUERTO"));*/

	suscribirseACola(socketBrokerAPP, APPEARED);
	/*suscribirseACola(socketBrokerLOC, LOCALIZED);
	suscribirseACola(socketBrokerCAU, CAUGHT);*/

	int socketBrokerParaMensaje = crearConexionCliente(ipServidor,puertoServidor);

    mensajeAppeared mensaje;
    mensaje.longPokemon=strlen("Pikachu")+1;
    mensaje.pokemon=malloc(mensaje.longPokemon);
    strcpy(mensaje.pokemon,"Pikachu");
    mensaje.posicionX=5;
    mensaje.posicionY=10;

	enviarMensajeABroker(socketBrokerParaMensaje, APPEARED, -1, sizeof(uint32_t)*3+mensaje.longPokemon, &mensaje);
    int idMensaje;
    recv(socketBrokerParaMensaje,&idMensaje,sizeof(int),MSG_WAITALL);
    log_debug(logger,"El broker ha respondido el ID del mensaje enviado: %d", idMensaje);

	//char* msjTest = malloc(sizeof("115elias"));
	//msjTest = 1 + 1 + "5elias"; // 115elias = MENSAJE NEW <sizeMsj> <Msj>
	//log_info(logger, msjTest);
	//enviarMensaje(socketBroker, msjTest);
	//enviarMensajeACola(socketBroker, NEW, "elias");

	//Procedimiento auxiliar para que no rompa el server en las pruebas
	/*int codigoOP = FINALIZAR;
	send(socketBrokerAPP,(void*)&codigoOP,sizeof(opCode),0);
	//send(socketBrokerLOC,(void*)&codigoOP,sizeof(opCode),0);
	//send(socketBrokerCAU,(void*)&codigoOP,sizeof(opCode),0);
    close(socketBrokerAPP);
    close(socketBrokerLOC);
    close(socketBrokerCAU);*/
    /*log_info(logger,"Finalizó la conexión con el servidor\n");
    log_info(logger,"El proceso team finalizó su ejecución\n");*/

    mensajeRecibido * msgRecibido = recibirMensajeDeBroker(socketBrokerAPP);
    mensajeAppeared* msjApp = malloc(sizeof(mensajeAppeared));
    int offset = 0;
    msjApp->pokemon=malloc(msgRecibido->sizeMensaje);

    memcpy(&(msjApp->longPokemon), msgRecibido->mensaje + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    log_info(logger, "Long: %d", &(msjApp->longPokemon));
    /*memcpy(msjApp->pokemon, msgRecibido->mensaje + offset, msjApp->longPokemon);
    offset += msjApp->longPokemon;
    log_info(logger, "Pokemon: %d", msjApp->pokemon);*/
    /*memcpy(&(msjApp.posicionX), msgRecibido->mensaje + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
    memcpy(&(msjApp.posicionY), msgRecibido->mensaje + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);*/

    log_info(logger,"Mensaje chorizeado: %s",(char *)msgRecibido->mensaje);

    /*log_info(logger, "Pokemon: %s",msjApp.pokemon);
    log_info(logger, "PosX: %d", msjApp.posicionX);
    log_info(logger, "PosY: %d", msjApp.posicionY);*/

	free(ipServidor);
	free(puertoServidor);
    log_destroy(logger);
    config_destroy(config);
    return 0;


}
