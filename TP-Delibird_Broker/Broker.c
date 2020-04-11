#include "Broker.h"

int main(){

	// 6 colas de mensajes - 3 pares de colas relacionadas
	//
	//inicializarColas();
	int socketEscucha = crearConexionServer("9898");
	int *socketCliente = esperarCliente(socketEscucha, 5);
	while(1){
		tPaquete *paquete = recibirMensaje(*socketCliente);
		printf("Código de operacion recibido: %d\n", paquete->codOperacion);
		printf("Buffer size recibido: %d\n", paquete->buffer->size);
		printf("Payload recibido: %s\n", (char *)paquete->buffer->stream);
		free(paquete->buffer->stream);
                free(paquete->buffer);
		free(paquete);
	}
	close(socketEscucha);
	close(*socketCliente);
	return 0;


}
