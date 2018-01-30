#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <semaphore.h>
#include <time.h>
#include <ncurses.h>

  ////////////////
 // ESTRUCTURAS//
////////////////

typedef struct{
    char** tablero;
    char** tab_inicial;
    int alto;
    int ancho;
    int zombies;
    int personas;
    int potencia_arma;
}board;

typedef struct{
    int posx;
    int posy;
    int armado;
    int balas;
    int esZombie;
    int vivo;
}humano;

typedef struct{
    int posx;
    int posy;
    int vivo;
}zombie;

typedef struct{
    board* B;
    humano* H;
}game;

typedef struct{
    board* B;
    zombie* Z;
}game_z;

typedef struct{
    int posx;
    int posy;
    int cola_zombies;
}respawn;

  //////////////////////////////
 // DECLARACION DE FUNCIONES //
//////////////////////////////

int evaluarContorno(board* juego, int posx, int posy, int *camino);
void iniciar_zombie(board* juego, zombie* zombie, pthread_t* hilo);
int esJugable_zombie(int posx, int posy, board* juego);
zombie* crear_zombie(int posx, int posy);
void* funcion_zombie(void* arg);
void tomar_Decision_zombie(board* juego, zombie* zombie);
int evaluarPosicion_zombie(board* juego, int posx, int posy, int camino);
respawn* crear_spawn(int posx, int posy);
humano* crear_humano(int posx, int posy);
void iniciar_humano(board* juego, humano* humano, pthread_t* hilo);
int evaluarPosicion(board* juego, int posx, int posy, int* camino);
void crear_Juego(char* archivo);
void mostrar_Tablero(board* B);
void* funcion_humano(void* arg);
int esJugable(int posx, int posy, board* juego);
void tomar_Decision(board* juego, humano* humano);
board* crear_Tablero(int ancho, int alto, int num_zombies, int num_personas, int potencia_armas);


  //////////
 // MAIN //
//////////
pthread_mutex_t lock;
int main(int argc, char *argv[]){
	srand(time(NULL)); /// SEMILLA RANDOM
	crear_Juego(argv[1]);
    return 0;
}


  ///////////////
 // FUNCIONES //
///////////////

//FUNCION QUE CREA UN TABLERO
board* crear_Tablero(int ancho, int alto, int num_zombies, int num_personas, int potencia_armas){
	board* T;
	int i;
	T = (board*)malloc(sizeof(board));
	char**tablero;
    tablero=(char**)malloc(alto*sizeof(char*)); //RESERVA MEMORIA AL TABLERO
    for(i=0; i<alto; i++)
    {
        tablero[i]=(char*)malloc(ancho*sizeof(char));
    }
    char **tab_inicial;
    tab_inicial=(char**)malloc(alto*sizeof(char*)); //RESERVA MEMORIA AL TABLERO
    for(i=0; i<alto; i++)
    {
        tab_inicial[i]=(char*)malloc(ancho*sizeof(char));
    }
    T->tablero=tablero;
    T->tab_inicial=tab_inicial;
    T->alto=alto;
    T->ancho=ancho;
    T->zombies=num_zombies;
    T->personas=num_personas;
    T->potencia_arma=potencia_armas;
    return T;
}

//FUNCION QUE INICIA EL JUEGO
void crear_Juego(char* archivo){
	FILE *fload;
	fload = fopen(archivo,"r");
    if(fload==NULL){
        printf("Error al leer el archivo, asegurese de escribir bien el nombre del archivo junto a su formato.\n");
        return;
    }
	int N, M, Z, P, B, i, j;
	fscanf(fload, "%d %d %d %d %d", &N, &M, &Z, &P, &B);
	// crea tablero
	board* Tablero;
	Tablero = crear_Tablero(N,M,Z,P,B);
    initscr();
	printw("N=%d\nM=%d\n\n",N,M);
	for(i=0;i<M;i++){ // LEE DATOS DEL TABLERO
		fscanf(fload,"%s",Tablero->tablero[i]);
		printw("%s\n",Tablero->tablero[i]);
    }
    pthread_t hilos_personas[P];
    pthread_t hilos_zombies[Z];
    humano* personas[P];
    zombie* zombies[Z];
    int num_E=0;
    int num_p=0;
    for(i=0;i<M;i++){
        for(j=0;j<N;j++){
            if(Tablero->tablero[i][j]=='E'){
                num_E++;
            }
        }
    }
    respawn* zonaSpawn[num_E];
    num_E=0;
    for(i=0;i<M;i++){
        for(j=0;j<N;j++){
            if(Tablero->tablero[i][j]=='P'){ // SE CREAN LAS PERSONAS
                Tablero->tab_inicial[i][j]='O';
                humano* H = crear_humano(j,i);
                personas[num_p]=H;
                printw("persona creada: %d\n",num_p );
                num_p++;
            }else
            if(Tablero->tablero[i][j]=='E'){ // SE CREA EL SPAWN
                respawn* spawn = crear_spawn(j,i);
                zonaSpawn[num_E]=spawn;
                Tablero->tab_inicial[i][j]=Tablero->tablero[i][j];
                num_E++;
            }else{
                Tablero->tab_inicial[i][j]=Tablero->tablero[i][j];
            }
        }
    }
    int zombies_creados=0;
    //CREA A LOS ZOMBIES
    while(1){
        for(i=0;i<num_E;i++){
            zombie* new_Zombie = crear_zombie(zonaSpawn[i]->posx,zonaSpawn[i]->posy);
            zombies[zombies_creados]=new_Zombie;
            printw("creado zombie %d\n",zombies_creados);
            zombies_creados++;
            if(zombies_creados==Z){
                break;
            }
        }
        if(zombies_creados==Z){
            break;
        }
    }

    for(i=0;i<P;i++){ //inicia a las personas en el tablero
        iniciar_humano(Tablero, personas[i], &hilos_personas[num_p]);
    }
    Tablero->personas=num_p;
    Tablero->zombies=0;
    printw("%d personas\n", num_p);
    time_t  tiempo_ini, tiempo_act;
    time(&tiempo_ini);
    int zombies_pendientes=Z;
    int cont=0;
    int zombies_antes=Tablero->zombies;
    int personas_antes=num_p;
    clear();
    refresh();
    start_color();
    init_pair(0, COLOR_BLACK, COLOR_BLACK);
    init_pair(1, COLOR_CYAN, COLOR_BLACK);
    init_pair(2, COLOR_RED, COLOR_BLACK);
    init_pair(3, COLOR_YELLOW, COLOR_BLACK);
    init_pair(4, COLOR_WHITE, COLOR_WHITE);
    init_pair(5, COLOR_GREEN, COLOR_BLACK);
    init_pair(6, COLOR_BLUE, COLOR_BLACK);
    while(1){
        if(cont==4 && zombies_pendientes>0){
            for (i = 0; i < num_E; ++i) // DESPLIEGA UN ZOMBIE CADA 3 SEGUNDOS DESDE UN SPAWN
            {
                if(zombies_pendientes>0){ 
                    iniciar_zombie(Tablero, zombies[zombies_pendientes-1], &hilos_zombies[zombies_pendientes-1]);
                    printw("Ha respawneado un zombie\n");
                    (Tablero->zombies)++;
                    zombies_antes=Tablero->zombies;
                    zombies_pendientes--;
                }
            }
            cont=0;
        }
        clear();
        //system("clear");
        time(&tiempo_act);
        mostrar_Tablero(Tablero);
        printw("Tiempo: %.2d:%.2d\n", (int)(tiempo_act-tiempo_ini)/60,(int)(tiempo_act-tiempo_ini)%60);
        printw("Personas: %d\n", Tablero->personas);
        printw("Zombies: %d \n", Tablero->zombies);
        if(personas_antes>Tablero->personas){
            if(personas_antes-(Tablero->personas)>1){
                printw("%d personas han muerto\n",personas_antes-(Tablero->personas));
                personas_antes=Tablero->personas;
            }else{
                printw("Una persona ha muerto\n");
                personas_antes=Tablero->personas;
            }
        }
        if(zombies_antes>Tablero->zombies){
            if(personas_antes-(Tablero->personas)>1){
                printw("%d zombies han sido exterminados\n",personas_antes-(Tablero->zombies));
                zombies_antes=Tablero->zombies;
            }else{
                printw("Un zombie ha sido exterminado\n");
                zombies_antes=Tablero->zombies;
            }
        }
        refresh();

        if(Tablero->personas==0){
            for(i=0;i<Tablero->alto;i++){
                for(j=0;j<Tablero->ancho;j++){
                    Tablero->tablero[i][j]='F';
                }
            }
            printw("\nFin del juego, la raza humana se ha extinguido\n");
            printw("\nPresione ENTER para continuar...\n");
            getch();
            break;
        }
        if(zombies_pendientes<=0 && Tablero->zombies<=0){
            for(i=0;i<Tablero->alto;i++){
                for(j=0;j<Tablero->ancho;j++){
                    Tablero->tablero[i][j]='F';
                }
            }
            printw("\nFin del juego, los humanos ganan!\n%d seres humanos han sobrevivido al apocalipsis zombie!\n",Tablero->personas);
            printw("\nPresione ENTER para continuar...\n");
            getch();
            break;
        }
        cont++;
    }
    endwin();
}

//CREA PUNTO DE SPAWN
respawn* crear_spawn(int posx, int posy){
    respawn * R;
    R = (respawn*)malloc(sizeof(respawn));
    R->posx=posx;
    R->posy=posy;
    R->cola_zombies=0;
    return R;
}

//CREA ZOMBIE
zombie* crear_zombie(int posx, int posy){
    zombie* Z;
    Z =(zombie*)malloc(sizeof(zombie));
    Z->posx=posx;
    Z->posy=posy;
    Z->vivo=1;
    return Z;
}

// FUNCION QUE REALIZA EL HILO DE LZOMBIE
void* funcion_zombie(void* arg){
    game_z* G = (game_z*) arg ;
    G->B->tablero[G->Z->posy][G->Z->posx]='Z';
    sleep(1);
    while(1){
        sleep(1);
        if(G->B->tablero[G->Z->posy][G->Z->posx]=='F'){
            break;
        }
        tomar_Decision_zombie(G->B,G->Z);
        if(G->Z->vivo==0){
            (G->B->zombies)--;
            sleep(3); // AL MORIR SU MARCA QUEDA POR 3 SEGUNDOS 'z'
            G->B->tablero[G->Z->posy][G->Z->posx]='O';
            break;
        }
    }
    return (void*)0;
}

//MUESTRA EL TABLERO CON NCURSES
void mostrar_Tablero(board* B){
    int i,j;
    sleep(1);
    for(i=0;i<B->alto;i++){
        for(j=0;j<B->ancho;j++){
            if(B->tablero[i][j]=='X'){
                attron(COLOR_PAIR(4));
                printw("%c",B->tablero[i][j]);
                attroff(COLOR_PAIR(4));

            }else
            if(B->tablero[i][j]=='O'){
                //attron(COLOR_PAIR(0));
                printw(" ",B->tablero[i][j]);
               // attroff(COLOR_PAIR(0));

            }else
            if(B->tablero[i][j]=='Z'){
                attron(COLOR_PAIR(2));
                printw("%c",B->tablero[i][j]);
                attroff(COLOR_PAIR(2));

            }else
            if(B->tablero[i][j]=='G'){
                attron(COLOR_PAIR(3));
                printw("%c",B->tablero[i][j]);
                attroff(COLOR_PAIR(3));

            }else
            if(B->tablero[i][j]=='P'){
                attron(COLOR_PAIR(5));
                printw("%c",B->tablero[i][j]);
                attroff(COLOR_PAIR(5));

            }else
            if(B->tablero[i][j]=='d'){
                attron(COLOR_PAIR(1));
                printw("%c",B->tablero[i][j]);
                attroff(COLOR_PAIR(1));

            }else
            if(B->tablero[i][j]=='z'){
                attron(COLOR_PAIR(1));
                printw("%c",B->tablero[i][j]);
                attroff(COLOR_PAIR(1));

            }else
            if(B->tablero[i][j]=='E'){
                attron(COLOR_PAIR(6));
                printw("_",B->tablero[i][j]);
                attroff(COLOR_PAIR(6));

            }else{
            printw("%c",B->tablero[i][j]);
            }
        }
        printw("\n");
    }
}

// INICIA UN HILO DE HUMANO
void iniciar_humano(board* juego, humano* humano, pthread_t* hilo){
    game* Game;
    Game=(game*)malloc(sizeof(game));
    Game->B=juego;
    Game->H=humano;
    pthread_create(hilo, NULL, funcion_humano,  ( void *) Game);
}

// INICIA UN HILO DE ZOMBIE
void iniciar_zombie(board* juego, zombie* zombie, pthread_t* hilo){
    game_z* Game;
    Game=(game_z*)malloc(sizeof(game_z));
    Game->B=juego;
    Game->Z=zombie;
    pthread_create(hilo, NULL, funcion_zombie,  ( void *) Game);
}

// FUNCION DEL HILO HUMANO
void* funcion_humano(void* arg){
    game* G = (game*) arg ;
    while(1){
        sleep(1);
        if(G->B->tablero[G->H->posy][G->H->posx]=='F'){
            break;
        }
        tomar_Decision(G->B,G->H);
        if(G->H->vivo==0){
            (G->B->zombies)--;
            sleep(3);
            G->B->tablero[G->H->posy][G->H->posx]='O';
            break;
        }
    }
    return (void*)0;
}

// CREA UN HUMANO
humano* crear_humano(int posx, int posy){
    humano* H;
    H = (humano*)malloc(sizeof(humano));
    H->posx=posx;
    H->posy=posy;
    H->armado=0;
    H->balas=0;
    H->esZombie=0;
    H->vivo=1;
    return H;
}


// TOMAR LA DECISION DE UN HUMANO
void tomar_Decision(board* juego, humano* humano){
    int decision, camino, dado;
    while(1){ // BUSCA UNA JUGADA VALIDA
        camino=rand()%8;
        if(humano->esZombie==0){
            pthread_mutex_lock(&lock);
            if(evaluarContorno(juego, humano->posx, humano->posy, &camino)==3){
                decision=3;
            }else{
                decision=evaluarPosicion(juego, humano->posx, humano->posy, &camino);
            }
            pthread_mutex_unlock(&lock);
        }else{
            if(juego->tablero[humano->posy][humano->posx]=='z'){
                humano->vivo=0;
                return;
            }
            decision=evaluarPosicion_zombie(juego, humano->posx, humano->posy, camino);
        }
        if(decision!=0){
            break;
        }
    }
    tomar_camino:
    //printw("tomando camino %d y decision %d\n",camino,decision );
    if(decision==1){
        if(camino==0){ //IZQ
            pthread_mutex_lock(&lock);
            juego->tablero[humano->posy][humano->posx]='O';
            humano->posx=(humano->posx)-1;
            if(humano->esZombie==1){
                juego->tablero[humano->posy][humano->posx]='Z';
            }else{
                juego->tablero[humano->posy][humano->posx]='P';
            }
            pthread_mutex_unlock(&lock);   
        }else
        if(camino==1){ // IZQ ARRIBA
            pthread_mutex_lock(&lock);
            juego->tablero[humano->posy][humano->posx]='O';
            humano->posx=(humano->posx)-1;
            humano->posy=(humano->posy)-1;
            if(humano->esZombie==1){
                juego->tablero[humano->posy][humano->posx]='Z';
            }else{
                juego->tablero[humano->posy][humano->posx]='P';
            }
            pthread_mutex_unlock(&lock); 
        }else
        if(camino==2){ // ARRIBA
            pthread_mutex_lock(&lock);
            juego->tablero[humano->posy][humano->posx]='O';
            humano->posy=(humano->posy)-1;
            if(humano->esZombie==1){
                juego->tablero[humano->posy][humano->posx]='Z';
            }else{
                juego->tablero[humano->posy][humano->posx]='P';
            }  
            pthread_mutex_unlock(&lock);

        }else
        if(camino==3){ // DER ARRIBA
            pthread_mutex_lock(&lock);
            juego->tablero[humano->posy][humano->posx]='O';
            humano->posx=(humano->posx)+1;
            humano->posy=(humano->posy)-1;
            if(humano->esZombie==1){
                juego->tablero[humano->posy][humano->posx]='Z';
            }else{
                juego->tablero[humano->posy][humano->posx]='P';
            }
            pthread_mutex_unlock(&lock);

        }else
        if(camino==4){ // DER
            pthread_mutex_lock(&lock);
            juego->tablero[humano->posy][humano->posx]='O';
            humano->posx=(humano->posx)+1;
            if(humano->esZombie==1){
                juego->tablero[humano->posy][humano->posx]='Z';
            }else{
                juego->tablero[humano->posy][humano->posx]='P';
            }
            pthread_mutex_unlock(&lock);

        }else
        if(camino==5){ // DER ABAJO
            pthread_mutex_lock(&lock);
            juego->tablero[humano->posy][humano->posx]='O';
            humano->posx=(humano->posx)+1;
            humano->posy=(humano->posy)+1;
            if(humano->esZombie==1){
                juego->tablero[humano->posy][humano->posx]='Z';
            }else{
                juego->tablero[humano->posy][humano->posx]='P';
            }
            pthread_mutex_unlock(&lock); 

        }else
        if(camino==6){ // ABAJO
            pthread_mutex_lock(&lock);
            juego->tablero[humano->posy][humano->posx]='O';
            humano->posy=(humano->posy)+1;
            if(humano->esZombie==1){
                juego->tablero[humano->posy][humano->posx]='Z';
            }else{
                juego->tablero[humano->posy][humano->posx]='P';
            }
            pthread_mutex_unlock(&lock); 

        }else
        if(camino==7){ // IZQ ABAJO
            pthread_mutex_lock(&lock);
            juego->tablero[humano->posy][humano->posx]='O';
            humano->posx=(humano->posx)-1;
            humano->posy=(humano->posy)+1;
            if(humano->esZombie==1){
                juego->tablero[humano->posy][humano->posx]='Z';
            }else{
                juego->tablero[humano->posy][humano->posx]='P';
            }
            pthread_mutex_unlock(&lock);
        }
    }else
    if(decision==2){
        pthread_mutex_lock(&lock);
        humano->armado=1;
        humano->balas=(humano->balas)+(juego->potencia_arma); // se acumulan las balas
        pthread_mutex_unlock(&lock);
    }else
    if(decision==3){
        dado=1+rand()%10; // SE TIRA EL DADO DE 10 CARAS
        if(dado>=4){ // SI DADO ES MAYOR O IGUAL A 4, LA PERSONA MUERE
            humano->vivo=0;
            humano->armado=0;
            humano->balas=0;
            pthread_mutex_lock(&lock);
            juego->tablero[humano->posy][humano->posx]='d';
            (juego->personas)--;
            pthread_mutex_unlock(&lock);
            sleep(3);
            humano->esZombie=1;
            humano->vivo=1;
            juego->tablero[humano->posy][humano->posx]='Z';
            (juego->zombies)++;
        }else{
            if(humano->armado==1 && humano->balas>0){ // SI ESTA ARMADO Y TIENE BALAS, MATA AL ZOMBIE
                pthread_mutex_lock(&lock);
                if(camino==0){ //IZQ
                    juego->tablero[humano->posy][(humano->posx)-1]='z';
                }else
                if(camino==1){ // IZQ ARRIBA
                    juego->tablero[(humano->posy)-1][(humano->posx)-1]='z';

                }else
                if(camino==2){ // ARRIBA
                    juego->tablero[(humano->posy)-1][humano->posx]='z'; 

                }else
                if(camino==3){ // DER ARRIBA
                    juego->tablero[(humano->posy)-1][(humano->posx)+1]='z'; 
                }else
                if(camino==4){ // DER
                    juego->tablero[humano->posy][(humano->posx)+1]='z';

                }else
                if(camino==5){ // DER ABAJO
                    juego->tablero[(humano->posy)+1][(humano->posx)+1]='z';
                }else
                if(camino==6){ // ABAJO
                    juego->tablero[(humano->posy)+1][humano->posx]='z';

                }else
                if(camino==7){ // IZQ ABAJO
                    juego->tablero[(humano->posy)+1][(humano->posx)-1]='z';
                }
                pthread_mutex_unlock(&lock);
            }else{ // SI ARRANCA
                if(camino==0){ // SI SE PUEDE CORRER EN DIRECCION CONTRARIA LO HACE, SI NO ELIGE UN CAMINO LIBRE ALEATORIO
                    if(juego->tablero[humano->posy][(humano->posx)+1]=='O'){
                        pthread_mutex_lock(&lock);
                        juego->tablero[humano->posy][humano->posx]='O';
                        humano->posx=(humano->posx)+1;
                        juego->tablero[humano->posy][humano->posx]='P'; 
                        pthread_mutex_unlock(&lock); 
                    }else{
                        while(1){
                            camino=rand()%8;
                            decision=evaluarPosicion(juego, humano->posx, humano->posy, &camino);
                            if(decision==1){
                                break;
                            }
                        }
                        goto tomar_camino;
                    }
                }else
                if(camino==1){
                    if(juego->tablero[(humano->posy)+1][(humano->posx)+1]=='O'){
                        pthread_mutex_lock(&lock);
                        juego->tablero[humano->posy][humano->posx]='O';
                        humano->posx=(humano->posx)+1;
                        humano->posx=(humano->posy)+1;
                        juego->tablero[humano->posy][humano->posx]='P'; 
                        pthread_mutex_unlock(&lock);
                    }else{
                        while(1){
                            camino=rand()%8;
                            decision=evaluarPosicion(juego, humano->posx, humano->posy, &camino);
                            if(decision==1){
                                break;
                            }
                        }
                        goto tomar_camino;
                    }

                }else
                if(camino==2){
                    if(juego->tablero[(humano->posy)+1][(humano->posx)]=='O'){
                        pthread_mutex_lock(&lock);
                        juego->tablero[humano->posy][humano->posx]='O';
                        humano->posx=(humano->posy)+1;
                        juego->tablero[humano->posy][humano->posx]='P';
                        pthread_mutex_unlock(&lock);
                    }else{
                        while(1){
                            camino=rand()%8;
                            decision=evaluarPosicion(juego, humano->posx, humano->posy, &camino);
                            if(decision==1){
                                break;
                            }
                        }
                        goto tomar_camino;
                    }

                }else
                if(camino==3){
                    if(juego->tablero[(humano->posy)+1][(humano->posx)-1]=='O'){
                        pthread_mutex_lock(&lock);
                        juego->tablero[humano->posy][humano->posx]='O';
                        humano->posx=(humano->posx)-1;
                        humano->posx=(humano->posy)+1;
                        juego->tablero[humano->posy][humano->posx]='P';
                        pthread_mutex_unlock(&lock);
                    }else{
                        while(1){
                            camino=rand()%8;
                            decision=evaluarPosicion(juego, humano->posx, humano->posy, &camino);
                            if(decision==1){
                                break;
                            }
                        }
                        goto tomar_camino;
                    }

                }else
                if(camino==4){
                    if(juego->tablero[humano->posy][(humano->posx)-1]=='O'){
                        pthread_mutex_lock(&lock);
                        juego->tablero[humano->posy][humano->posx]='O';
                        humano->posx=(humano->posx)-1;
                        juego->tablero[humano->posy][humano->posx]='P';
                        pthread_mutex_unlock(&lock); 
                    }else{
                        while(1){
                            camino=rand()%8;
                            decision=evaluarPosicion(juego, humano->posx, humano->posy, &camino);
                            if(decision==1){
                                break;
                            }
                        }
                        goto tomar_camino;
                    }

                }else
                if(camino==5){
                    if(juego->tablero[(humano->posy)-1][(humano->posx)-1]=='O'){
                        pthread_mutex_lock(&lock);
                        juego->tablero[humano->posy][humano->posx]='O';
                        humano->posx=(humano->posx)-1;
                        humano->posx=(humano->posy)-1;
                        juego->tablero[humano->posy][humano->posx]='P';
                        pthread_mutex_unlock(&lock); 
                    }else{
                        while(1){
                            camino=rand()%8;
                            decision=evaluarPosicion(juego, humano->posx, humano->posy, &camino);
                            if(decision==1){
                                break;
                            }
                        }
                        goto tomar_camino;
                    }

                }else
                if(camino==6){
                    if(juego->tablero[(humano->posy)-1][(humano->posx)]=='O'){
                        pthread_mutex_lock(&lock);
                        juego->tablero[humano->posy][humano->posx]='O';
                        humano->posx=(humano->posy)-1;
                        juego->tablero[humano->posy][humano->posx]='P';
                        pthread_mutex_unlock(&lock); 
                    }else{
                        while(1){
                            camino=rand()%8;
                            decision=evaluarPosicion(juego, humano->posx, humano->posy, &camino);
                            if(decision==1){
                                break;
                            }
                        }
                        goto tomar_camino;
                    }

                }else
                if(camino==7){
                    if(juego->tablero[(humano->posy)-1][(humano->posx)+1]=='O'){
                        pthread_mutex_lock(&lock);
                        juego->tablero[humano->posy][humano->posx]='O';
                        humano->posx=(humano->posx)+1;
                        humano->posx=(humano->posy)-1;
                        juego->tablero[humano->posy][humano->posx]='P';
                        pthread_mutex_unlock(&lock); 
                    }else{
                        while(1){
                            camino=rand()%8;
                            decision=evaluarPosicion(juego, humano->posx, humano->posy, &camino);
                            if(decision==1){
                                break;
                            }
                        }
                        goto tomar_camino;
                    }
                }
                
            }
            //...
        }
    }
    //pthread_mutex_unlock(&lock); 
}

//TOMA LA DECISION DEL ZOMBIE
void tomar_Decision_zombie(board* juego, zombie* zombie){
    int decision, camino;
    while(1){ // BUSCA UNA JUGADA VALIDA
        camino=rand()%8;
        
        if(juego->tablero[zombie->posy][zombie->posx]=='z'){
            zombie->vivo=0;
            return;
        }
        decision=evaluarPosicion_zombie(juego, zombie->posx, zombie->posy, camino);
        if(decision!=0){
            break;
        }
    }
    pthread_mutex_lock(&lock);
    if(decision==1){
        if(camino==0){//IZQ
            if(juego->tab_inicial[zombie->posy][zombie->posx]=='E'){
               juego->tablero[zombie->posy][zombie->posx]='E'; 
            }else{
                juego->tablero[zombie->posy][zombie->posx]='O';
            }
            zombie->posx=(zombie->posx)-1;
            juego->tablero[zombie->posy][zombie->posx]='Z';
        }else
        if(camino==1){ // IZQ ARRIBA
            if(juego->tab_inicial[zombie->posy][zombie->posx]=='E'){
               juego->tablero[zombie->posy][zombie->posx]='E'; 
            }else{
                juego->tablero[zombie->posy][zombie->posx]='O';
            }
            zombie->posx=(zombie->posx)-1;
            zombie->posy=(zombie->posy)-1;
            juego->tablero[zombie->posy][zombie->posx]='Z';
        }else
        if(camino==2){ // ARRIBA
            if(juego->tab_inicial[zombie->posy][zombie->posx]=='E'){
               juego->tablero[zombie->posy][zombie->posx]='E'; 
            }else{
                juego->tablero[zombie->posy][zombie->posx]='O';
            }
            zombie->posy=(zombie->posy)-1;
            juego->tablero[zombie->posy][zombie->posx]='Z';

        }else
        if(camino==3){ // DER ARRIBA
            if(juego->tab_inicial[zombie->posy][zombie->posx]=='E'){
               juego->tablero[zombie->posy][zombie->posx]='E'; 
            }else{
                juego->tablero[zombie->posy][zombie->posx]='O';
            }
            zombie->posx=(zombie->posx)+1;
            zombie->posy=(zombie->posy)-1;
            juego->tablero[zombie->posy][zombie->posx]='Z';

        }else
        if(camino==4){ // DER
            if(juego->tab_inicial[zombie->posy][zombie->posx]=='E'){
               juego->tablero[zombie->posy][zombie->posx]='E'; 
            }else{
                juego->tablero[zombie->posy][zombie->posx]='O';
            }
            zombie->posx=(zombie->posx)+1;
            juego->tablero[zombie->posy][zombie->posx]='Z';

        }else
        if(camino==5){ // DER ABAJO
            if(juego->tab_inicial[zombie->posy][zombie->posx]=='E'){
               juego->tablero[zombie->posy][zombie->posx]='E'; 
            }else{
                juego->tablero[zombie->posy][zombie->posx]='O';
            }
            zombie->posx=(zombie->posx)+1;
            zombie->posy=(zombie->posy)+1;
            juego->tablero[zombie->posy][zombie->posx]='Z';

        }else
        if(camino==6){ // ABAJO
            if(juego->tab_inicial[zombie->posy][zombie->posx]=='E'){
               juego->tablero[zombie->posy][zombie->posx]='E'; 
            }else{
                juego->tablero[zombie->posy][zombie->posx]='O';
            }
            zombie->posy=(zombie->posy)+1;
            juego->tablero[zombie->posy][zombie->posx]='Z';

        }else
        if(camino==7){ // IZQ ABAJO
            if(juego->tab_inicial[zombie->posy][zombie->posx]=='E'){
               juego->tablero[zombie->posy][zombie->posx]='E'; 
            }else{
                juego->tablero[zombie->posy][zombie->posx]='O';
            }
            zombie->posx=(zombie->posx)-1;
            zombie->posy=(zombie->posy)+1;
            juego->tablero[zombie->posy][zombie->posx]='Z';
        }
    }
    pthread_mutex_unlock(&lock);
}


//EVALUA SI HAY ZOMBIES ALREDEDOR
int evaluarContorno(board* juego, int posx, int posy, int *camino){
     if(esJugable(posx-1, posy, juego)==3)
    {
        *camino=0; // SE MODIFICA EL CAMINO PARA ENFRENTAR AL ZOMBIE
        return 3;
    }else 
    if(esJugable(posx-1, posy-1, juego)==3)
    {
        *camino=1;
        return 3;
    }else 
    if(esJugable(posx, posy-1, juego)==3)
    {
        *camino=2;
        return 3;
    }else 
    if(esJugable(posx+1, posy-1, juego)==3)
    {
        *camino=3;
        return 3;
    }else 
    if(esJugable(posx+1, posy, juego)==3)
    {
        *camino=4;
        return 3;
    }else 
    if(esJugable(posx+1, posy+1, juego)==3)
    {
        *camino=5;
        return 3;
    }else 
    if(esJugable(posx, posy+1, juego)==3)
    {
        *camino=6;
        return 3;
    }else 
    if(esJugable(posx-1, posy+1, juego)==3)
    {
        *camino=7;
        return 3;
    }
    return 0;
}

//EVALUA EL CAMINO SELECCIONADO
int evaluarPosicion(board* juego, int posx, int posy, int *camino){
    // SI NO HAY ZOMBIES ALREDEDOR
    if(*camino==0){// izquierda
        if(esJugable(posx-1, posy, juego)==0){
            return 0;
        }else 
        if(esJugable(posx-1, posy, juego)==1){
            return 1;
        }else
        if(esJugable(posx-1, posy, juego)==2){
            return 2;
        }else
        if(esJugable(posx-1, posy, juego)==3){
            return 3;
        }
    }
    if(*camino==1){// IZQ arriba
        if(esJugable(posx-1, posy-1, juego)==0){
            return 0;
        }else 
        if(esJugable(posx-1, posy-1, juego)==1){
            return 1;
        }else
        if(esJugable(posx-1, posy-1, juego)==2){
            return 2;
        }else
        if(esJugable(posx-1, posy-1, juego)==3){
            return 3;
        }
    }
    if(*camino==2){// arriba
        if(esJugable(posx, posy-1, juego)==0){
            return 0;
        }else 
        if(esJugable(posx, posy-1, juego)==1){
            return 1;
        }else
        if(esJugable(posx, posy-1, juego)==2){
            return 2;
        }else
        if(esJugable(posx, posy-1, juego)==3){
            return 3;
        }
    }
    if(*camino==3){// DER arriba
        if(esJugable(posx+1, posy-1, juego)==0){
            return 0;
        }else 
        if(esJugable(posx+1, posy-1, juego)==1){
            return 1;
        }else
        if(esJugable(posx+1, posy-1, juego)==2){
            return 2;
        }else
        if(esJugable(posx+1, posy-1, juego)==3){
            return 3;
        }
    }
    if(*camino==4){// derecha
        if(esJugable(posx+1, posy, juego)==0){
            return 0;
        }else 
        if(esJugable(posx+1, posy, juego)==1){
            return 1;
        }else
        if(esJugable(posx+1, posy, juego)==2){
            return 2;
        }else
        if(esJugable(posx+1, posy, juego)==3){
            return 3;
        }
    }
    if(*camino==5){// DER abajo 
        if(esJugable(posx+1, posy+1, juego)==0){
            return 0;
        }else 
        if(esJugable(posx+1, posy+1, juego)==1){
            return 1;
        }else
        if(esJugable(posx+1, posy+1, juego)==2){
            return 2;
        }else
        if(esJugable(posx+1, posy+1, juego)==3){
            return 3;
        }
    }
    if(*camino==6){// abajo
        if(esJugable(posx, posy+1, juego)==0){
            return 0;
        }else 
        if(esJugable(posx, posy+1, juego)==1){
            return 1;
        }else
        if(esJugable(posx, posy+1, juego)==2){
            return 2;
        }else
        if(esJugable(posx, posy+1, juego)==3){
            return 3;
        }
    }
    if(*camino==7){// IZQ abajo 
        if(esJugable(posx-1, posy+1, juego)==0){
            return 0;
        }else 
        if(esJugable(posx-1, posy+1, juego)==1){
            return 1;
        }else
        if(esJugable(posx-1, posy+1, juego)==2){
            return 2;
        }else
        if(esJugable(posx-1, posy+1, juego)==3){
            return 3;
        }
    }
    return -1;
}

//EVALUA LA POSICION DEL ZOMBIE
int evaluarPosicion_zombie(board* juego, int posx, int posy, int camino){
    if(camino==0){// izquierda
        if(esJugable_zombie(posx-1, posy, juego)==0){
            return 0;
        }else 
        if(esJugable_zombie(posx-1, posy, juego)==1){
            return 1;
        }
    }
    if(camino==1){// IZQ arriba
        if(esJugable_zombie(posx-1, posy-1, juego)==0){
            return 0;
        }else 
        if(esJugable_zombie(posx-1, posy-1, juego)==1){
            return 1;
        }
    }
    if(camino==2){// arriba
        if(esJugable_zombie(posx, posy-1, juego)==0){
            return 0;
        }else 
        if(esJugable_zombie(posx, posy-1, juego)==1){
            return 1;
        }
    }
    if(camino==3){// DER arriba
        if(esJugable_zombie(posx+1, posy-1, juego)==0){
            return 0;
        }else 
        if(esJugable_zombie(posx+1, posy-1, juego)==1){
            return 1;
        }
    }
    if(camino==4){// derecha
        if(esJugable_zombie(posx+1, posy, juego)==0){
            return 0;
        }else 
        if(esJugable_zombie(posx+1, posy, juego)==1){
            return 1;
        }
    }
    if(camino==5){// DER abajo 
        if(esJugable_zombie(posx+1, posy+1, juego)==0){
            return 0;
        }else 
        if(esJugable_zombie(posx+1, posy+1, juego)==1){
            return 1;
        }
    }
    if(camino==6){// abajo
        if(esJugable_zombie(posx, posy+1, juego)==0){
            return 0;
        }else 
        if(esJugable_zombie(posx, posy+1, juego)==1){
            return 1;
        }
    }
    if(camino==7){// IZQ abajo 
        if(esJugable_zombie(posx-1, posy+1, juego)==0){
            return 0;
        }else 
        if(esJugable_zombie(posx-1, posy+1, juego)==1){
            return 1;
        }
    }
    return -1;
}

//VE SI LA JUGADA ES POSIBLE
int esJugable(int posx, int posy, board* juego){
    if(posx<0 || posy>juego->alto || posy<0 || posx>juego->ancho){
        return 0;
    }
    if(juego->tablero[posy][posx]=='O'){
        return 1;
    }
    if(juego->tablero[posy][posx]=='G'){
        return 2;
    }
    if(juego->tablero[posy][posx]=='Z'){ // se encuentra con un zombie
        return 3;
    }else{
        return 0;
    }
}

// VE SI LA JUGADA DEL ZOMBIE ES POSIBLE
int esJugable_zombie(int posx, int posy, board* juego){
    if(posx<0 || posy>juego->alto || posy<0 || posx>juego->ancho){
        return 0;
    }
    if(juego->tablero[posy][posx]=='O'){
        return 1;
    }
    else{
        return 0;
    }
}
