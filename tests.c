#include <stdlib.h>
#include <stdio.h>
#include "CUnit/Basic.h"
//#include "Team.h"

/* TESTS TEAMS */
void testPosicionesSeArmanBien(){
	t_entrenador* testEntrenador = malloc(sizeof(t_entrenador));
	t_list* posiciones = list_create();
	t_list* objetivosEntrenador = list_create();
	t_list* pokemonesEntrenador = list_create();

	list_add(posiciones, "1");
	list_add(posiciones, "2");
	list_add(objetivosEntrenador, "pikachu|charmander");
	list_add(pokemonesEntrenador, "charmander|bulbasaur|squirtle");


	testEntrenador = armarEntrenador(list_get(posiciones, 0), objetivosEntrenador, pokemonesEntrenador);
	CU_ASSERT_EQUAL(1, testEntrenador->pos[0]);
}
void testObjetivosSeArmanBien(){
	t_entrenador* testEntrenador = malloc(sizeof(t_entrenador));
	char* posiciones = malloc(sizeof(char)*2);
	char* objetivosEntrenador = malloc(sizeof("pikachu|charmander")+1);
	char* pokemonesEntrenador = malloc(sizeof("charmander|bulbasaur|squirtle")+1);

	posiciones[0] = '1';
	posiciones[1] = '2';
	strcpy(objetivosEntrenador, "pikachu|charmander");
	strcpy(pokemonesEntrenador, "charmander|bulbasaur|squirtle");

	testEntrenador = armarEntrenador(posiciones, objetivosEntrenador, pokemonesEntrenador);
	CU_ASSERT_EQUAL(1, testEntrenador->pos[0]);
}

int main(){

	CU_initialize_registry();

	/*** TESTS ***/

	/* TESTS TEAMS */
	CU_pSuite team = CU_add_suite ("Teams", NULL, NULL);
	CU_add_test(team, "la abscisa de la posicion del entrenador se carga correctamente", testPosicionesSeArmanBien);


	/*** END TESTS ***/

	CU_basic_set_mode(CU_BRM_VERBOSE);
	CU_basic_run_tests();
	CU_cleanup_registry();

	return CU_get_error();
}


