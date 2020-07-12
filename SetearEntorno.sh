pathTP='/home/utnso'

echo "######################## TP DELIBIRD - Ripped Dinos ##########################"
echo "Creando carpeta /home/utnso/commons/ ..."
sudo mkdir "${pathTP}/commons/"

# echo "Clonando repo de TP..."
# git clone https://github.com/sisoputnfrba/tp-2020-1c-Ripped-Dinos

echo "Clonando repo de commons..."
sudo git clone https://github.com/sisoputnfrba/so-commons-library "${pathTP}/commons/"

echo "Instalando commons..."
cd  "${pathTP}/commons/"
sudo make install 

echo "Compilando sharedLib..."
cd "${pathTP}/tp-2020-1c-Ripped-Dinos/TP-Delibird_SharedLib/Debug/"
sudo make all 

echo "Compilando broker..."
cd "${pathTP}/tp-2020-1c-Ripped-Dinos/TP-Delibird_Broker/Debug/"
sudo make all 

echo "Compilando team..."
cd "${pathTP}/tp-2020-1c-Ripped-Dinos/TP-Delibird_Team/Debug/"
sudo make all 

echo "Compilando gamecard..."
cd "${pathTP}/tp-2020-1c-Ripped-Dinos/TP-Delibird_Game-Card/Debug/"
sudo make all 

echo "Compilando gameboy..."
cd "${pathTP}/tp-2020-1c-Ripped-Dinos/TP-Delibird_Game-Boy/Debug/"
sudo make all 

echo "Seteando variable de entorno"
sudo echo "export LD_LIBRARY_PATH=/home/utnso/tp-2020-1c-Ripped-Dinos/TP-Delibird_SharedLib/Debug/" >> ~/.bashrc


