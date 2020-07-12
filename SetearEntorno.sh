pathTP='/home/utnso'

echo "TP DELIBIRD - Ripped Dinos"
echo "Creando carpeta /home/utnso/ ..."
mkdir "${pathTP}/commons/"

echo "Clonando repo de TP..."
git clone https://github.com/sisoputnfrba/tp-2020-1c-Ripped-Dinos

echo "Clonando repo de commons..."
git clone https://github.com/sisoputnfrba/so-commons-library "${pathTP}/commons/"

echo "Instalando commons..."
cd  "${pathTP}/commons/"
sudo make install 

echo "Compilando sharedLib..."
cd "${pathTP}/tp-2020-1c-Ripped-Dinos/TP-Delibird_SharedLib/Debug/"
make all 

echo "Compilando broker..."
cd "${pathTP}/tp-2020-1c-Ripped-Dinos/TP-Delibird_Broker/Debug/"
make all 

echo "Compilando team..."
cd "${pathTP}/tp-2020-1c-Ripped-Dinos/TP-Delibird_Team/Debug/"
make all 

echo "Compilando gamecard..."
cd "${pathTP}/tp-2020-1c-Ripped-Dinos/TP-Delibird_Game-Card/Debug/"
make all 

echo "Compilando gameboy..."
cd "${pathTP}/tp-2020-1c-Ripped-Dinos/TP-Delibird_Game-Boy/Debug/"
make all 

echo "Seteando variable de entorno"
export LD_LIBRARY_PATH=/home/utnso/tp-2020-1c-Ripped-Dinos/TP-Delibird_SharedLib/Debug/


