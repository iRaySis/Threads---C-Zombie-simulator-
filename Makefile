DIR_OUTPUT = ./SimuladorZombie
DIR_SRC = ./src
NOMBRE_MAIN = simulador_zombie
FLAG = -Wall -lpthread -lncurses
GCC = gcc

all: dir main

dir: 
# Crear carpeta"Simulador Zombie"
	mkdir -p $(DIR_OUTPUT)

main:
	$(GCC) $(NOMBRE_MAIN).c -o $(DIR_OUTPUT)/$(NOMBRE_MAIN) $(FLAG)

#clean:
#	rm -rf $(DIR_OUTPUT)/*.o
