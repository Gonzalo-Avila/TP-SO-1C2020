#include "Broker.h"


void empezarAAtenderCliente(int socketEscucha) {
	pthread_t hiloAtenderCliente;
	pthread_create(&hiloAtenderCliente, NULL, (void*) atenderConexiones,
			&socketEscucha);
	pthread_detach(hiloAtenderCliente);
}

void atenderConexiones(int *socketEscucha) {
	/* Espera nuevas conexiones en el socket de escucha. Al establecerse una nueva, envía esa conexión a un nuevo hilo para que
	 * sea gestionada y vuelve a esperar nuevas conexiones.
	 */
	int backlog_server = config_get_int_value(config, "BACKLOG_SERVER");
	atenderConexionEn(*socketEscucha, backlog_server);
	while (1) {
		log_debug(logger, "Esperando cliente...");
		int *socketCliente = esperarCliente(*socketEscucha);
		log_info(logger,
				"Se ha conectado un cliente. Número de socket cliente: %d",
				*socketCliente);

        esperarMensajes(socketCliente);
        free(socketCliente);
	}
}



void esperarMensajes(int *socketCliente) {
	/* Espera mensajes de una conexión ya establecida. Según el código de operación recibido, delega tareas a distintos modulos.
	 *
	 */

	opCode codOperacion;
	int sizeDelMensaje;
	cola tipoCola;

	recv(*socketCliente, &codOperacion, sizeof(opCode), MSG_WAITALL);
	log_debug(logger, "Esperando mensaje de cliente %d...", *socketCliente);


	switch (codOperacion) {

	case NUEVA_CONEXION:{
        uint32_t idProceso= getIDProceso();
        send(*socketCliente, &idProceso,sizeof(uint32_t),0);
        close(*socketCliente);
		break;
	}

	case SUSCRIPCION: {
		log_info(logger, "[SUSCRIPCION]");
		atenderSuscripcion(socketCliente);
		log_info(logger, "[SUSCRIPCION-END]");
		break;
	}

	case NUEVO_MENSAJE: {
		log_info(logger, "[NUEVO_MENSAJE]");
		recv(*socketCliente, &sizeDelMensaje, sizeof(uint32_t), MSG_WAITALL);
		recv(*socketCliente, &tipoCola, sizeof(cola), MSG_WAITALL);
		atenderMensaje(*socketCliente, tipoCola);

		close(*socketCliente);
		break;
	}
	case FINALIZAR: {
		/* Finaliza la conexión con el broker de forma ordenada.
		 * No creo que tenga mucho sentido en el TP, seria para hacer pruebas.
		 *
		 *
		 */
		log_info(logger, "[FINALIZAR]");
		recv(*socketCliente, &tipoCola, sizeof(cola), MSG_WAITALL);
		uint32_t clientID;
		recv(*socketCliente, &clientID, sizeof(uint32_t), MSG_WAITALL);
		desuscribir(clientID, tipoCola);
		log_info(logger, "El cliente con ID %d se ha desconectado",
				clientID);

		break;
	}
	default: {
		log_error(logger, "El mensaje recibido está dañado");
		break;
	}
	}
}


bool yaExisteSuscriptor(uint32_t clientID, cola codSuscripcion){
    bool existeClientID(void * nodoLista){
		   suscriptor* sus = (suscriptor *) nodoLista;
		   return sus->clientID==clientID;
	}
   t_list * listaSuscriptores = getListaSuscriptoresByNum(codSuscripcion);

   return list_any_satisfy(listaSuscriptores,(void *)existeClientID);;
}



void atenderSuscripcion(int *socketSuscriptor){
	/* Recibe el código de suscripción desde el socket a suscribirse, eligiendo de esta manera la cola y agregando el socket
	 * a la lista de suscriptores de la misma.
	 */
	cola codSuscripcion;
	uint32_t sizePaquete;
	suscriptor * nuevoSuscriptor = malloc(sizeof(suscriptor));
	nuevoSuscriptor->socketCliente=*socketSuscriptor;

	recv(*socketSuscriptor, &sizePaquete, sizeof(uint32_t), MSG_WAITALL);
	recv(*socketSuscriptor, &codSuscripcion, sizeof(cola), MSG_WAITALL);
	recv(*socketSuscriptor, &(nuevoSuscriptor->clientID), sizeof(uint32_t), MSG_WAITALL);

	log_debug(logger, "%d", codSuscripcion);

	sem_wait(&mutexColas);
	if(yaExisteSuscriptor(nuevoSuscriptor->clientID,codSuscripcion)==true){
		suscriptor * suscriptorYaAlmacenado = buscarSuscriptor(nuevoSuscriptor->clientID,codSuscripcion);
        suscriptorYaAlmacenado->socketCliente=nuevoSuscriptor->socketCliente;

        log_info(logger,
        				"El cliente %d ha actualizado el socket: %d",
						suscriptorYaAlmacenado->clientID, suscriptorYaAlmacenado->socketCliente);

        //TODO - COMENTAR ESTA LINEA Y REALIZAR TESTING
		//enviarMensajesCacheados(suscriptorYaAlmacenado, codSuscripcion);
	}
	else
	{
		list_add(getListaSuscriptoresByNum(codSuscripcion), nuevoSuscriptor);
		log_info(logger,
				"Hay un nuevo suscriptor en la cola %s. Número de socket suscriptor: %d",
				getCodeStringByNum(codSuscripcion), *socketSuscriptor);

		//enviarMensajesCacheados(nuevoSuscriptor, codSuscripcion);
	}
	sem_post(&mutexColas);

}

void atenderMensaje(int socketEmisor, cola tipoCola) {
	int idMensaje;
	uint32_t idCorrelativo;
	recv(socketEmisor, &idCorrelativo, sizeof(uint32_t), MSG_WAITALL);

	if (tipoCola >= 0 && tipoCola <= 5) {
		idMensaje = agregarMensajeACola(socketEmisor, tipoCola, idCorrelativo);
		send(socketEmisor, &idMensaje, sizeof(uint32_t), 0);
	} else {
		log_error(logger, "[ERROR]");
		log_error(logger,
				"No pudo obtenerse el tipo de cola en el mensaje recibido");
	}
}


void imprimirEstructuraDeDatos(estructuraMensaje mensaje) {
	log_info(logger, "[NUEVO MENSAJE RECIBIDO]");
	log_info(logger, "ID: %d", mensaje.id);
	log_info(logger, "ID correlativo: %d", mensaje.idCorrelativo);
	log_info(logger, "Tamaño de mensaje: %d", mensaje.sizeMensaje);
	log_info(logger, "Tipo mensaje: %s", getCodeStringByNum(mensaje.colaMensajeria));
}

estructuraMensaje * generarNodo(estructuraMensaje mensaje) {

	estructuraMensaje * nodo = malloc(sizeof(estructuraMensaje));
	nodo->mensaje = malloc(mensaje.sizeMensaje);

	nodo->id = mensaje.id;
	nodo->idCorrelativo = mensaje.idCorrelativo;
	nodo->estado = mensaje.estado;
	nodo->sizeMensaje = mensaje.sizeMensaje;
	nodo->mensaje = mensaje.mensaje;
	nodo->colaMensajeria = mensaje.colaMensajeria;
	nodo->clientID = mensaje.clientID;
	return nodo;
}

int agregarMensajeACola(int socketEmisor, cola tipoCola, int idCorrelativo) {

	estructuraMensaje mensajeNuevo;

	recv(socketEmisor, &mensajeNuevo.sizeMensaje, sizeof(int), MSG_WAITALL);
	mensajeNuevo.mensaje = malloc(mensajeNuevo.sizeMensaje);
	recv(socketEmisor, mensajeNuevo.mensaje, mensajeNuevo.sizeMensaje, MSG_WAITALL);

	uint32_t id = getIDMensaje();

	mensajeNuevo.id = id;
	mensajeNuevo.idCorrelativo = idCorrelativo;
	mensajeNuevo.estado = ESTADO_NUEVO;
	mensajeNuevo.colaMensajeria = tipoCola;

	imprimirEstructuraDeDatos(mensajeNuevo);

	sem_wait(&mutexColas);
	for (int i = 0; i < list_size(getListaSuscriptoresByNum(tipoCola)); i++) {

		suscriptor* sus;
		sus = (suscriptor *) (list_get(getListaSuscriptoresByNum(tipoCola), i));
		mensajeNuevo.clientID = sus->clientID;
		list_add(getColaByNum(tipoCola), generarNodo(mensajeNuevo));

		cachearMensaje(mensajeNuevo.id,mensajeNuevo.idCorrelativo, mensajeNuevo.colaMensajeria, mensajeNuevo.sizeMensaje,mensajeNuevo.mensaje);
	}
	sem_post(&mutexColas);
	sem_post(&habilitarEnvio);

	return id;
}