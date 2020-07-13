pathTP='/home/utnso'

echo "######################## TP DELIBIRD - Ripped Dinos ##########################"
echo "Clonando repo de commons..."
sudo git clone https://github.com/sisoputnfrba/so-commons-library "${pathTP}/"

echo "Instalando commons..."
cd  "${pathTP}/so-commons-library/"
sudo make install

echo "Compilando Shared Library..."
cd "${pathTP}/tp-2020-1c-Ripped-Dinos/TP-Delibird_SharedLib/Debug/"
sudo make clean
sudo make all

echo "Compilando broker..."
cd "${pathTP}/tp-2020-1c-Ripped-Dinos/TP-Delibird_Broker/Debug/"
sudo make clean
sudo make all

echo "Compilando team..."
cd "${pathTP}/tp-2020-1c-Ripped-Dinos/TP-Delibird_Team/Debug/"
sudo make clean
sudo make all

echo "Compilando gamecard..."
cd "${pathTP}/tp-2020-1c-Ripped-Dinos/TP-Delibird_Game-Card/Debug/"
sudo make clean
sudo make all

echo "Compilando gameboy..."
cd "${pathTP}/tp-2020-1c-Ripped-Dinos/TP-Delibird_Game-Boy/Debug/"
sudo make clean
sudo make all

echo "Seteando variable de entorno"
sudo echo "export LD_LIBRARY_PATH=/home/utnso/tp-2020-1c-Ripped-Dinos/TP-Delibird_SharedLib/Debug/" >> ~/.bashrc
