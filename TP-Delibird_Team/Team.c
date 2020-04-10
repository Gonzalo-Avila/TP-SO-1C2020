#include "Team.h"


int main(){

	int socketServidor = crearConexionCliente("9898", "127.0.0.1");
    while(1){
    	char * mensaje = malloc(1024);
      	printf("Ingrese mensaje: ");
      	scanf("%s",mensaje);
      	enviarMensaje(mensaje,socketServidor);
    }
    close(socketServidor);
    return 0;


}
