pathTP='/home/utnso/test'


echo "TP DELIBIRD - Ripped Dinos"

echo "Creando carpeta /home/utnso/ ..."
mkdir "${pathTP}
mkdir "${pathTP}/tp/"
mkdir "${pathTP}/commons/

echo "Clonando repo de TP..."
git clone https://github.com/sisoputnfrba/tp-2020-1c-Ripped-Dinos "${pathTP}/tp/"

echo "Clonando repo de commons..."
git clone https://github.com/sisoputnfrba/so-commons-library "${pathTP}/commons/"

echo "Instalando commons... "
cd  "${pathTP}/commons/"
make install 

echo "Seteando variable de entorno"
export LD_LIBRARY_PATH="${pathTP}/tp/tp-2020-1c-Ripped-Dinos/TP-Delibird_SharedLib/Debug/"

echo "Compilando sharedLib..."
cd "${pathTP}/tp/tp-2020-1c-Ripped-Dinos/TP-Delibird_SharedLib/Debug/"
make all 

echo "Compilando broker..."
cd "${pathTP}/tp/tp-2020-1c-Ripped-Dinos/TP-Delibird_Broker/Debug/"
make all 

echo "Compilando team..."
cd "${pathTP}/tp/tp-2020-1c-Ripped-Dinos/TP-Delibird_Team/Debug/"
make all 

echo "Compilando gamecard..."
cd "${pathTP}/tp/tp-2020-1c-Ripped-Dinos/TP-Delibird_Game-Card/Debug/"
make all 

echo "Compilando gameboy..."
cd "${pathTP}/tp/tp-2020-1c-Ripped-Dinos/TP-Delibird_Game-Boy/Debug/"
make all 
