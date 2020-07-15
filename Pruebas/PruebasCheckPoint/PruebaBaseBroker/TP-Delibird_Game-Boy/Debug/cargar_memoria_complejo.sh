#!/bin/sh
./TP-Delibird_Game-Boy BROKER CATCH_POKEMON Pikachu 9 3 #19b
./TP-Delibird_Game-Boy BROKER CATCH_POKEMON Squirtle 9 3 #20b

./TP-Delibird_Game-Boy BROKER CAUGHT_POKEMON 10 OK
./TP-Delibird_Game-Boy BROKER CAUGHT_POKEMON 11 FAIL

./TP-Delibird_Game-Boy BROKER CATCH_POKEMON Bulbasaur 1 7 #21
./TP-Delibird_Game-Boy BROKER CATCH_POKEMON Charmander 1 7 #22

./TP-Delibird_Game-Boy BROKER GET_POKEMON Pichu #9
./TP-Delibird_Game-Boy BROKER GET_POKEMON Raichu #10
