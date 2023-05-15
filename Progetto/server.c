

/*
																	PROGETTO OS_STORE
																	FABIO SIRUGO
																	CORSO B
																	MAT: 533099						
*/



#define _POSIX_SOURCE 200809L
#define _GNU_SOURCE
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/un.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <pthread.h>
#include "time.h"


/*

															NOTA: 
										PRIMA DI AVVIARE UN QUALSIASI TEST ESEGUIRE 
										1) make clean 
										2) make					
										3) make nometest
										

*/

/*
Most filesystems will have some limits on files, such as a maximum length on components in a file path.
The limit on path components (also known as the file name limit) is defined as NAME_MAX , generally 255 bytes.
*/

/*
MAXIMUM_PATH : Utilizzato per ottenere un percorso.
MAX_CLIENT_IN : il numero massimo di client accettati.
SIZE_HEADER : La dimensione MASSIMA che posso avere in un header considerando spazi.

*/

/*															DEFINE Valori														*/
#define CONNECT_MAX 30
#define CONNECT_MIN 28
#define MAXIMUM_PATH 4096
#define SOCKNAME "objstore.sock"
#define SIZE_HEADER 16
#define MAX_SIZE_FILE 255
#define BLOCCO_DATI 2048
//per vedere le stampe del server basta settare VERBOSE a 1
#define VERBOSE 0
/*															Variabili globali													*/
/*
			Una volta avviato il server tengo traccia di anomalie, file e byte salvati.

client_totali : dice il numero di client che hanno usato il server
client_connessi : dice quanti client sono connessi in un determinato momento.
stored_Data : il numero di byte salvati all'interno di OS_STORE
numero_file : numero di file presenti in totale. (tiene traccia anche quando alcuni vengono cancellati)
path_iniziale : Utilizzata per tracciare il percorso fino a ..../data
fine : variabile per verificare quando il server dovrà fermarsi.

*/

int client_connessi=0;
pthread_attr_t thattr;
int client_totali;
int client_connessi;
long stored_Data;
int fd_skt;
char *path_iniziale;
int numero_file;
int numero_anomalie_client;
int numero_fileRimossi;
int self_pipe[2];
volatile sig_atomic_t show_stats;
volatile sig_atomic_t fine;
static pthread_cond_t controllo = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t numero_fileRimossi_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t anomalie_client_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t client_connessi_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t numero_file_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t stored_Data_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t client_totali_mutex = PTHREAD_MUTEX_INITIALIZER;

/*															Firme Metodi																						*/

int readn(long fd, void *buf, size_t size);
static void gestore_segnali(int signum);
void inc_client_connessi();
void dec_client_connessi();
void inc_numero_anomalie_client();
void inc_numero_nuovi_file();
void inc_numero_fileRimossi();
void dec_stored_data(int size);
void inc_stored_data(int size);
void inc_client_totali();
void cleanup();
void cleanpath();
void start_server();
int writen(long fd, void *buf, size_t size);		
char *createPath(char *s1 , char *s2);
char *createData();
char *createDir(char *path , char *nome );
void dispatcher (int skt);
static void* gestore_Clienti(void*arg);
void createThread(long fd_conn);
char **tokenizer(char *str, int *j);
int myLeave(int fd , char*);
int myStore(int fd , char *path);
int myRetrieve(int fd , char *path);
int myDelete(int fd , char *path);
void signal_main();
void * signal_handler_thread();


/*------------------------------------------------------------------------------------------------------------------------------*/


/*																main 																					*/


int main(){

	atexit(cleanup);
	atexit(cleanpath);
/*												Creazione socket 																 */

	start_server();
	struct sigaction s;
	memset(&s , 0 , sizeof(s));
	s.sa_handler = gestore_segnali;
	if(sigaction(SIGUSR1 , &s ,NULL) == -1)		perror("Errore sigaction SIGUSR1");
	if(sigaction(SIGQUIT , &s ,NULL) == -1)		perror("Errore sigaction SIGQUIT");
	if(sigaction(SIGINT  , &s ,NULL) == -1)		perror("Errore sigaction SIGINT");
	if(sigaction(SIGPIPE , &s ,NULL) == -1)		perror("Errore sigaction SIGPIPE");
	if(sigaction(SIGTERM , &s ,NULL) == -1)		perror("Errore sigaction SIGTERM");


/*												Creazione Directory Data 														 */
	if((path_iniziale = createData()) == NULL){
		perror("Errore crezione Data");
		return 0;
	}

/*												Creazione pipe e creazione thread_segnali 	  									 */
	pthread_t handler_thread;
	if(pipe(self_pipe) == -1) {
		perror("Errore pipe");
		return 0;
	}
	if(pthread_create(&handler_thread, NULL, signal_handler_thread, NULL) == -1) {
		perror("Errore create signal");
		return 0;
	}
	signal_main();
/*												Dispatcher Thread 																  */
	dispatcher(fd_skt);
	pthread_join(handler_thread, NULL);
	return 0;

}




/*-------------------------------------------------- Implementazioni Metodi ------------------------------------------------------*/

void * signal_handler_thread(){
	
	fd_set rd_set, fset;
	FD_ZERO(&fset);
	FD_SET(self_pipe[0], &fset);
	char letto;
	int esito_select;
	while(!fine){
		rd_set = fset;
		if(show_stats){

			printf("\n                SHOW STATS\nNumero client attivi al momento : %d\nNumero Byte in + o in - all'interno di Data : %ld\nNumero File salvati in totale : %d\nNumero file rimossi : %d\nNumero file in + o in - all'interno di Data : %d\nNumero client che hanno utilizzato il server : %d\n",client_connessi , stored_Data , numero_file,numero_fileRimossi ,(numero_file-numero_fileRimossi),client_totali);
			printf("numero_anomalie_client -> %d\n",numero_anomalie_client);
			show_stats = 0;
		}
		//questa select è usata per generare errore quando un segnale la interrompe
		esito_select = select(self_pipe[0]+1, &rd_set, NULL, NULL, NULL);
		if(esito_select > 0)
			esito_select = readn(self_pipe[0], (void*)(&letto), 1);
	}
	if(show_stats){

		printf("\n                SHOW STATS\nNumero client attivi al momento : %d\nNumero Byte in + o in - all'interno di Data : %ld\nNumero File salvati in totale : %d\nNumero file rimossi : %d\nNumero file in + o in - all'interno di Data : %d\nNumero client che hanno utilizzato il server : %d\n",client_connessi , stored_Data , numero_file,numero_fileRimossi ,(numero_file-numero_fileRimossi),client_totali);
		printf("numero_anomalie_client -> %d\n",numero_anomalie_client);
		show_stats = 0;
	}
	close(self_pipe[0]);
	close(self_pipe[1]);
	pthread_exit(0);
}






/*												Funzione per avviare il socket  												*/
void gestore_segnali(int signum){
	if(writen(self_pipe[1], "0", 1) == -1 )
		perror("Errore write self-pipe");
	if(SIGUSR1 == signum){
		show_stats = 1;
	}
	else if(signum == SIGPIPE){}
	else if(signum == SIGTERM || signum == SIGQUIT || signum == SIGINT){
		fine = 1;
	}

}


//		 Utilizzo pthread_sigmask.
//       A new thread inherits a copy of its creator's signal mask.

void signal_main() {
	sigset_t maschera;
	sigfillset(&maschera);
	pthread_sigmask(SIG_SETMASK, &maschera, NULL);

}




// Avvio il server 


void start_server(){
	
	unlink(SOCKNAME);
	struct sockaddr_un sa;
	strncpy(sa.sun_path, SOCKNAME ,sizeof(sa.sun_path));
	sa.sun_family=AF_UNIX;

	//Creo il socket usando il file descriptor skt
	if((fd_skt=socket(AF_UNIX,SOCK_STREAM,0)) == -1){
		perror("Errore nella creazione socket Server");
		exit(EXIT_FAILURE);
	}
	//Fase di Bind in cui inizializzo sa
	if(bind(fd_skt,(struct sockaddr *)&sa , sizeof(sa)) == -1){
		perror("Errore nel bind socket Server");
		exit(EXIT_FAILURE);
	}
	//Fase di listen in cui il processo padre (server) si mette in attesa di qualche messaggio
	if(listen(fd_skt,SOMAXCONN)== -1){
		perror("Errore nel bind socket Server");
		exit(EXIT_FAILURE);
	}
}
/*													Funzioni Utile scrittura e lettura							*/

int writen(long fd, void *buf, size_t size) {
	size_t left = size;
	int r;
	char *bufptr = (char*)buf;
	while(left>0) {
		if ((r=write((int)fd ,bufptr,left)) == -1) {
			if (errno == EINTR) continue;
			return -1;
		}
		if (r == 0) return 0;  //non dovrebbe mai verificarsi, a meno che left non sia 0, condizione esclusa dal while.
		left    -= r;
		bufptr  += r;
	}
	return 1;
}
/*
																myRead 
Prende in input il file descriptor del socket.
L'idea di questa funzione è quella di leggere a blocchi di BLOCCO_DATI byte alla volta fino a quando non si trova il carattere che definisce la
fine della comunicazione ovvero il new line '\n'.
Viene utilizzata principalmente nella Store e nella Retrieve.
Ovviamente nella Store e nella Retrieve è possibile leggere oltre \n ma questo non sarà un problema perchè avrò letto anche len. Mi basta solo trovare 
la differenza di byte letti e byte che ancora dovrò leggere (se ci sono) dopo \n.  
*/


char *myRead(int fd){
	
	char *buffer = calloc(BLOCCO_DATI , sizeof(char));
	int i = 0;
	int trovato = 0;
	int bytetotali = 0;
	int byteletti = 0;
	int lim = 0;
	char *message = malloc(1);
	message[0] = '\0';
	//Una Read che va fino in fondo a cercare lo \n
	while(!trovato){
		if((byteletti = read(fd , buffer , BLOCCO_DATI)) < 0){
			inc_numero_anomalie_client();
			perror("Errore nella read myRead");
			pthread_exit(0);						
		}
		lim = bytetotali;
		bytetotali += byteletti;
		i = lim;
		while(i < bytetotali && !trovato){
			if(buffer[i] == '\n') trovato = 1;
			i++;
		}
		if(byteletti == 0 && !trovato)			break;
		message = realloc(message , bytetotali * sizeof(char)+1);
		strncat(message , buffer , byteletti);
		buffer = memset(buffer, 0, BLOCCO_DATI);
	}
	free(buffer);
	return message;

}

/*											Metodi per la gestione delle Directory 												*/

/*Linux has a maximum filename length of 255 characters for most filesystems (including EXT4), and a maximum path of 4096 characters.*/



//Crea un path 
//Fare la Free dopo la chiamata !
char *createPath(char *s1 , char *s2){

	char *pathData = malloc(MAXIMUM_PATH);
	strcpy(pathData, strcat(s1, "/"));
	char *temp = malloc(MAXIMUM_PATH);
	strcpy(temp , pathData);
	strcpy(pathData, strcat(temp, s2));
	free(temp);
	return pathData;
}

//Crea la directory iniziale Data e restituisce il path
char *createData(){	
	struct stat st = {0};
	char *path_tmp = malloc(MAXIMUM_PATH);
	if(path_tmp == NULL){
		inc_numero_anomalie_client();
		perror("Errore creazione Directory Data malloc");
		return NULL;
	}
	char *pathData = getcwd(path_tmp, MAXIMUM_PATH);
	//funzione che salva il path corrente in buff.
	pathData = createPath(path_tmp , "data");
	//Ora concateno il vecchio path con la directory che voglio creare
	if (stat(pathData, &st) == -1) {
    	mkdir(pathData, 0700);
	}
	free(path_tmp);
	return pathData;
}

void cleanpath(){
	free(path_iniziale);
}
void cleanup(){
	unlink(SOCKNAME);
}



/*													Funzioni per i Thread 													*/



/*
														dispatcher
La funzione dispatcher inizializza le variabili globali ed un attributo di tipo che utilizzerò alla creazione dei thread.(PTHREAD_CREATE_DETACHED).
Dopo le fasi di inizializzazione entra in un ciclo che termina solo quando riceve l'apposito segnale di terminazione.
Effettua la accept ottenendo così un fd che viene passato come argomento al thread appena creato.
Come scelta implementativa ho introdotto una variabile di condizionamento che mette in " wait il ciclo "  nel caso in cui il numero di client attivi in quel momento
è alto. Ho fatto questa scelta implementativa perchè notavo che con un numero molto alto di clienti il sever rallentava molto.
*/

void dispatcher (int fd) {

	show_stats = 0;
	fine = 0;
	numero_anomalie_client = 0;
	stored_Data = 0;
	client_connessi = 0;
	numero_file = 0;
	numero_fileRimossi = 0;
	long confd;
	pthread_t id_Thread;
	struct timeval timeout;
	fd_set fset, rd_set;
	
	while (!fine){
		if(VERBOSE)
			printf("Sono nel dispatcher\n");
		
		FD_ZERO(&fset);
		FD_SET(fd, &fset);
		timeout.tv_usec = 1000;
		rd_set = fset;
		if(select(fd+1, &rd_set, NULL, NULL, &timeout) > 0){
			errno = 0;
			if((confd = accept(fd,NULL,NULL)) == -1){
				if (errno == EINTR) continue;
				fine = 1;
			}
			if(!fine && errno != EINTR){
				inc_client_connessi();
				
				if(pthread_create(&id_Thread ,&thattr , *gestore_Clienti , (void*) confd) != 0){
					perror("Fail creazione thread");
					close(confd);
					dec_client_connessi();
				}
			}
		}

	}
	pthread_mutex_lock(&client_connessi_mutex);
	while(client_connessi > 0){
		pthread_cond_wait(&controllo , &client_connessi_mutex);
	}
	pthread_mutex_unlock(&client_connessi_mutex);
	if(VERBOSE)
			printf("\nEseguire make clean prima di un nuovo testscript per mantenere le statistiche consistenti\n");
	pthread_attr_destroy(&thattr);
}

/*
											tokenizer
funzione che prende in input una stringa ed un intero. La stringa verrà tokenizzata eseguendo uno split su spazi e \n 
l'intero mi dirà il numero di token che sono stati generati.
*/

char **tokenizer(char *str , int *j){
	char *state;
	if(str == NULL){
		perror("Errore stringa NULLA");
		return NULL;
	}
	char *token = __strtok_r(str, " \n", &state);
	char **mess = NULL;
	int i = 0;
	while(token){
		mess = realloc(mess , (i+1)*sizeof(char*) );
		if(mess == NULL){
			inc_numero_anomalie_client();
			perror("Errore nella calloc : tokenizer");
			return NULL;
		}
		mess[i]=calloc(strlen(token)+1,sizeof(char));
		strcpy(mess[i],token);
		i++;
		token = __strtok_r(NULL," \n",&state);
	}
	*j = i;
	return mess;
}


/*
															gestore_Clienti
Thread per la gestione dei client.
Come primo passo si assicura che il primo comando sia effettivamente una Register.
Dopo di che entra in un ciclo in cui controlla la prossima operazione da eseguire.
*/
static void *gestore_Clienti(void*arg){
	
/* 													Inizio del thread aspetto la REGISTER 								*/	
	long fd = (long) arg;
	if(VERBOSE)
		printf("Client connessi -> %d\n",client_connessi);
	char *nome = NULL;
	//Come primo comando aspetto una Register di conseguenza setto la dimensione del buffer considerando la massima dimensione del nome di
	// un file in Linux.
	//SIZE_HEADER contiene il numero di Byte necessari a contenere una "REGISTER LunghezzaNome \n"   
	int dim_buf = 12 + MAX_SIZE_FILE;
	char *buffer = calloc(dim_buf, sizeof(char));
	memset(buffer , 0 , dim_buf);
	int byteletti;
	if(buffer == NULL){
		perror("Errore nella calloc1 gestore_Clienti");
		close(fd);
		dec_client_connessi();
		pthread_exit(0);
	}
	if((byteletti = read(fd , buffer , dim_buf)) < 0){
		inc_numero_anomalie_client();
		perror("Errore nella read1 gestore_Clienti");
		close(fd);
		dec_client_connessi();
		pthread_exit(0);
	}
	if(byteletti == 0){
		inc_numero_anomalie_client();
		perror("Nome nullo o troppo lungo gestore_Clienti");
		close(fd);
		free(buffer);
		dec_client_connessi();
		pthread_exit(0);
	}
	char dirname_path[MAXIMUM_PATH];
	int number_of_token = 0;
	char **split_to_space =tokenizer(buffer , &number_of_token);
	free(buffer);
	/*													OS_CONNECT																*/
	inc_client_totali();
	int errore = 0;
	if((strncmp(split_to_space[0], "REGISTER", 8) == 0)){		
		nome = split_to_space[1];
		if(nome == NULL ||strlen(nome) >= MAX_SIZE_FILE){
			inc_numero_anomalie_client();
			if((writen(fd , "KO registrazione fallita nome non valido \n", 42)) == -1){
				inc_numero_anomalie_client();
				perror("Errore nella write1 REGISTER");
				dec_client_connessi();
				pthread_exit(0); 
			} 
		}
		//Ho ricevuto REGISTER e va avanti
		if((writen(fd , "OK \n", 4)) == -1){
			inc_numero_anomalie_client();
			perror("Errore nella write3 REGISTER");
			errore = 1;
		}
		
		if(VERBOSE)
			printf("%s %s\n",split_to_space[0] , split_to_space[1]);
		time_t tick;
		tick=time(NULL);
		if(VERBOSE)
			printf("Client %s registrato in data %s\n", split_to_space[1] ,ctime(&tick));
	}
	else{
		inc_numero_anomalie_client();
		perror("Comando non riconosciuto register");
		if((writen(fd , "KO comando non riconosciuto \n", 25)) == -1)
		errore = 1;
	}
	if(dirname_path == NULL){
		inc_numero_anomalie_client();
		perror("Qualcosa è andato storto con il path_iniziale");
		exit(EXIT_FAILURE);
	}
	if(errore){
		inc_numero_anomalie_client();
		close(fd);
		dec_client_connessi();
		pthread_exit(0);
	}

	//Creo il percorso per creare la Directory
	
	strcpy(dirname_path , path_iniziale);
	strcat(dirname_path , "/");
	strcat(dirname_path , split_to_space[1]);
	errno = 0;
	if(mkdir(dirname_path,0777) == -1){
		if(errno != EEXIST){
			free(split_to_space[0]);
			free(split_to_space[1]);
			free(split_to_space);
			perror("Errore creazione directory"); 
			return NULL;
		} //se name esiste già non è un problema
	}
	//clean tokens && Buffer
	free(split_to_space[0]);
	free(split_to_space);
	int termina = 0;
	char *new_buffer = calloc(1 , sizeof(char));
	if(new_buffer == NULL){
		inc_numero_anomalie_client();
		perror("Errore nella calloc");
		return 0;
	}
/*_______________________________________________________________________________________________________________________________________________*/

/*______________________A questo punto ho il nome del client non dovrei più ricevere una Register________________________________________________*/

/*_______________________________________________________________________________________________________________________________________________*/

	int letti = 0;
	if(fine){
		free(nome);
		free(new_buffer);
		dec_client_connessi();
		pthread_exit(0);
	}
	//Ciclo per capire quale comando ricevo.
	while(!termina){
		if(fine || termina){
			free(nome);
			free(new_buffer);
			dec_client_connessi();
			pthread_exit(0);
		}
		memset(new_buffer , 0 ,1);
		if((letti = read(fd , new_buffer , 1)) < 0){
			perror("Errore nella read ciclo termina");
			if (errno == EINTR){
				errno = 0;
				letti = read(fd , new_buffer , 1);
			}
			else {dec_client_connessi(); pthread_exit(0);}
		}

		if(new_buffer[0] == 'L'){
			free(new_buffer);
			myLeave(fd , nome);
			free(nome);
			dec_client_connessi();
			pthread_exit(0);
		}
		else if(new_buffer[0] == 'D'){
			myDelete(fd, nome);
		}
		else if(new_buffer[0] == 'R'){
			myRetrieve(fd, nome);
		}
		else if(new_buffer[0] == 'S'){
			myStore(fd, nome);
		}
	}
	dec_client_connessi();
	pthread_exit(0);
}

/*_______________________________________________________________________________________________________________________________________________*/

/*_____________________________________________________Funzioni per gestire i comandi____________________________________________________________*/

/*_______________________________________________________________________________________________________________________________________________*/



/*	
															myDelete								
Funzione per gestire la os_delete lato server.
Questa prima controlla che effettivamente il comando arrivato sia una Delete.
Successivamente verifica l'esistenza o meno del file da cancellare, se il file esite effettua la cancellazione e manda un response positivo al client,
altrimenti manda un response di fallimento.
*/

int myDelete(int fd , char *client){
	if(fd < 0){
		perror("Non ancora connesso myDelete");
		return 0;
	}
	char *cmd = calloc(6 , sizeof(char));
	cmd = memset(cmd , 0 , 6);
	if((read(fd , cmd , 6)) < 0){
		inc_numero_anomalie_client();
		perror("Errore nella read myDelete");
		dec_client_connessi();
		pthread_exit(0);						
	}
	if(strncmp(cmd , "ELETE " , 6) != 0){
		inc_numero_anomalie_client();
		if((writen(fd , "KO comando non riconosciuto \n", 29)) == -1){
			inc_numero_anomalie_client();
			perror("Errore nella write myDelete");
			return 0;
		}
	}
	if(VERBOSE)
		printf("DELETE ");
	free(cmd);
	char *message = calloc(MAX_SIZE_FILE + 2 , sizeof(char));
	if(message == NULL){
		perror("Errore calloc myDelete");
		inc_numero_anomalie_client();
		if((writen(fd , "KO errore calloc \n", 18)) == -1){
			inc_numero_anomalie_client();
			perror("Errore nella write myDelete");
			return 0;
		}
		return 0;
	}
	if((read(fd , message , (MAX_SIZE_FILE + 2))) < 0){
		inc_numero_anomalie_client();
		perror("Errore nella read myDelete");
		return 0;	
	}
	int num = 0;
	char **tok = tokenizer(message ,&num);
	free(message);
	//Numero di token sbagliato, comando errato
	if(num != 1){
		inc_numero_anomalie_client();
		perror("Errore in tokenizer");
		if((writen(fd , "KO in tokenizer \n", 17)) == -1){
			inc_numero_anomalie_client();
			perror("Errore nella write myDelete");
			return 0;
		}
		return 0;
	}
	char *nome_file = tok[0];
	if(VERBOSE)
		printf("%s\n",nome_file);
	char new_path[MAX_SIZE_FILE];
	strcpy(new_path , path_iniziale);
	strcat(new_path , "/");
	strcat(new_path , client);
	strcat(new_path , "/");
	strcat(new_path , nome_file);
	int size_file;
	FILE *fd_f;
	if ((fd_f=fopen(new_path,"r"))==NULL) {
		if(VERBOSE)
			printf("%s Non presente all'interno di %s \n\n",nome_file , client);
		if((writen(fd , "KO file non esistente \n", 23)) == -1){
			inc_numero_anomalie_client();
			perror("Errore nella write myDelete");
			free(nome_file);
			free(tok);
			return 0; 
		} 
		free(nome_file);
		free(tok);
		return 0;
		//Fare le free di tutto
	}
	//Controllo la dimensione del file, non eseguo il rewind perchè il file tanto va eliminato.
	fseek(fd_f, 0L, SEEK_END);
	size_file = ftell(fd_f);

	int esito = remove(new_path);
	fclose(fd_f);
	if(!esito){
		dec_stored_data(size_file);
		inc_numero_fileRimossi();
		if((writen(fd , "OK \n", 4)) == -1){
			inc_numero_anomalie_client();
			perror("Errore nella write myDelete");
			free(nome_file);
			free(tok);
			return 0;
		}
		if(VERBOSE)
			printf("DELETE OK\n\n");
		free(nome_file);
		free(tok);
		return 1;
	}
	else{
		if((writen(fd , "KO delete fallita \n", 19)) == -1){
			inc_numero_anomalie_client();
			perror("Errore nella write myDelete");
			free(nome_file);
			free(tok);
			return 0;
		}
		free(nome_file);
		free(tok);
		return 0;
	}
}
/*
																myStore
Dopo essersi assicurati che il comando sia effettivamente una Store chiama myRead e legge fino ad incontrare \n.
A questo punto se i caratteri letti dopo \n sono uguali a len allora ho ricevuto il file per intero, altrimenti 
dovrò leggere ancora.
*/

int myStore(int fd , char *client){
	if(fd < 0){
		perror("Non ancora connesso myStore");
		return 0;
	}
	char *cmd = calloc(6 , sizeof(char));
	if(cmd == NULL){
		inc_numero_anomalie_client();
		perror("Errore calloc myStore");
		if((writen(fd , "KO errore calloc \n", 18)) == -1){
			inc_numero_anomalie_client();
			perror("Errore nella write myStore");
			return 0;
		}
		return 0;
	}
	if(client == NULL){
		perror("Client NULL");
		return 0;
	}
	if((read(fd , cmd , 5)) < 0){
		inc_numero_anomalie_client();
		perror("Errore nella read myStore");
		return 0;					
	}
	if(strncmp(cmd , "TORE ",5) != 0){
		inc_numero_anomalie_client();
		if((writen(fd , "KO comando non riconosciuto \n", 29)) == -1){
			inc_numero_anomalie_client();
			perror("Errore nella write myStore");
			return 0;
		}
		return 0;
	}
	if(VERBOSE)
		printf("S%s",cmd); 
	free(cmd);
/*						A questo punto ho la certezza che il comando è una Store 													*/

	char *message = myRead(fd);
	if(message == NULL){
		inc_numero_anomalie_client();
		if((writen(fd , "KO errore in lettura \n", 22)) == -1){
			inc_numero_anomalie_client();
			perror("Errore nella write myStore");
			return 0;
		}
		return 0;
	}
	int num = 0;
	char **tok = tokenizer(message ,&num);
	//Numero di token sbagliato, comando errato
	if(num != 3){
		inc_numero_anomalie_client();
		perror("Errore in tokenizer");
		if((writen(fd , "KO errore in lettura \n", 22)) == -1){
			inc_numero_anomalie_client();
			perror("Errore nella write myStore");
			return 0;
		}
		return 0;
	}
	free(message);
	char *nome_file = tok[0];
	char *lunghezza_letta = tok[1];
	void *split_data = tok[2];
	if(VERBOSE)
		printf("%s\n",nome_file);
	int numero_caratteri_letti = strlen(split_data);
	int numero_caratteri_totali = atoi(lunghezza_letta);
	int dif = numero_caratteri_totali - numero_caratteri_letti;
	char *file_to_store = NULL;
	
	// A questo punto se dif != 0 vuol dire che ho letto tutto il file
	//Altrimenti leggo ancora la parte mancante ed utilizzo read che legge esattamente n byte

	if(dif == 0)	file_to_store = split_data;
	else{
		if(((file_to_store = calloc(numero_caratteri_totali+1 , sizeof(void))) == NULL)){
			perror("Errore nella calloc");
			return 0;
		}
		memset(file_to_store , 0 , numero_caratteri_totali + 1);
		strcpy(file_to_store , split_data);
		char *read_more = calloc(dif+1 , sizeof(void));
		if(read_more == NULL){
			inc_numero_anomalie_client();
			if((writen(fd , "KO errore calloc \n",18)) == -1){
				inc_numero_anomalie_client();
				free(file_to_store);
				perror("Errore nella write1 myStore");
				return 0;
			} 
		}
		memset(read_more , 0 , dif);
		if(readn(fd , read_more , dif) < 0){
			inc_numero_anomalie_client();
			if((writen(fd , "KO errore in lettura \n", 22)) == -1){
				free(file_to_store);
				inc_numero_anomalie_client();
				perror("Errore nella write myStore");
				return 0; 
			}
		}
		strcat(file_to_store , read_more);
		free(read_more);
	}

	char new_path[MAXIMUM_PATH];
	strcpy(new_path , path_iniziale);
	strcat(new_path , "/");
	strcat(new_path , client);
	strcat(new_path , "/");
	strcat(new_path , nome_file);

	//Se il file è già presente lo elimino perchè sovrascrivendolo corro il riscio di lasciare il file in modo inconsistente.
	if( access( new_path, F_OK ) != -1 ) {
		remove(new_path);
	}
	int fd_file = open(new_path  , O_CREAT | O_WRONLY ,0777);
	if(fd_file < 0 ){
		perror("Errore apertura file");
		if((writen(fd , "KO errore aperture file \n", 25)) == -1){
			perror("Errore nella write");
			inc_numero_anomalie_client();
			return 0;
		}
	}
	//Uso writen perchè ho la dimensione esatta del file.
	if((writen(fd_file , file_to_store , numero_caratteri_totali)) == -1){
		if((writen(fd , "KO errore scrittura \n", 21)) == -1){
			perror("Errore nella write");
			inc_numero_anomalie_client();
			return 0;
		}
		perror("Errore nella writen");
		inc_numero_anomalie_client();
		return 0;
	}
	close(fd_file);
	if((writen(fd , "OK \n", 4)) == -1){
		perror("Errore nella write");
		inc_numero_anomalie_client();
		return 0;
	}

	inc_numero_nuovi_file();
	inc_stored_data(numero_caratteri_totali);

	for(int i = 0 ; i < num ; i++){
		free(tok[i]);
	}
	free(tok);	
	if(dif != 0)	free(file_to_store);
	if(VERBOSE)
		printf("STORE OK\n\n");
	return 1;
}

/*
	La myRetrieve dopo aver ricevuto il comando Retrieve apre il file richeisto dal client lo salva in un buffer.
	A questo punto crea la stringa per rispondere al server, 
	La stringa da inviare contiene DATA len /n file_letto
	
*/
int myRetrieve(int fd , char *client){

	if(fd < 0){
		perror("Non ancora connesso myRetrieve");
		return 0;
	}
	char *cmd = calloc(MAX_SIZE_FILE + 12 , sizeof(char));
	errno = 0;
	if(cmd == NULL){
		inc_numero_anomalie_client();
		free(cmd);
		perror("Errore calloc myRetrieve");
		if((writen(fd , "KO errore calloc \n", 18)) == -1){
			perror("Errore nella write myRetrieve");
			inc_numero_anomalie_client();
			return 0;
		}
		return 0;
	}
	if(read(fd , cmd , MAX_SIZE_FILE + 12) < 0){
		inc_numero_anomalie_client();
		perror("Errore in letture");
		free(cmd);
		return 0;
	}
	int num_token = 0;
	char **tok_header = tokenizer(cmd , &num_token);
	free(cmd);
	if(strncmp(tok_header[0] , "ETRIEVE" , 7) != 0){
		perror("Comando errato");
		inc_numero_anomalie_client();
		if((writen(fd , "KO comando non valido \n", 23)) == -1){
			perror("Errore nella read");
			inc_numero_anomalie_client();
			return 0;
		}
		return 0;
	}
	if(VERBOSE)
		printf("R%s %s\n",tok_header[0] , tok_header[1]);
	//Il comando è una Retrieve
	char *nome_file = tok_header[1];
	char new_path[MAXIMUM_PATH];
	strcpy(new_path , path_iniziale);
	strcat(new_path , "/");
	strcat(new_path , client);
	strcat(new_path , "/");
	strcat(new_path , nome_file);
	for(int i = 0 ; i < num_token ; i++){
		free(tok_header[i]);
	}
	free(tok_header);
	FILE *fd_f;
	if ((fd_f=fopen(new_path,"r"))==NULL) {	
		perror("File non esistente");
		if(VERBOSE)
			printf("\n");
		if((writen(fd , "KO apertura file RETRIEVE\n", 26)) == -1){
			inc_numero_anomalie_client();
			perror("Errore nella write1");
			return 0;
		}

		return 0;
	}
	//Invece di leggere alla cieca calcolo prima la lunghezza e poi faccio una sola read.
	long long size_file = 0;
	fseek(fd_f, 0L, SEEK_END);
	size_file = ftell(fd_f);
	fseek(fd_f , 0L , SEEK_SET);
	fclose(fd_f);
	char *tmp = calloc(size_file +1,sizeof(char));
	if(tmp == NULL){
		perror("Errore nella calloc");
		return 0;
	}
	sprintf(tmp , "%lld", size_file);
	char *buffer_file = calloc(size_file+1 , sizeof(char));
	if(buffer_file == NULL){
		perror("Errore calloc myRetrieve");
		inc_numero_anomalie_client();
		free(tmp);
		if((writen(fd , "KO errore calloc \n", 18)) == -1){
			inc_numero_anomalie_client();
			perror("Errore nella write2");
			return 0;
		}
		return 0;
	}
	memset(buffer_file , 0 , size_file+1);
	errno = 0;
	int fd_file = open(new_path , O_RDONLY);
	if(fd_file == -1){
		perror("Errore apertura file con SC");
		if((writen(fd , "KO errore apertura \n", 20)) == -1){
			perror("Errore nella write3");
			inc_numero_anomalie_client();		
			return 0;
		}
	}

	if(readn(fd_file , buffer_file, size_file) == -1){
		perror("Errore nella readn");
		if((writen(fd , "KO errore lettura file \n", 24)) == -1){
			perror("Errore nella write3");
			inc_numero_anomalie_client();		
			return 0;
		}	
	}
	if(size_file != strlen(buffer_file)){
		perror("Errore nella writen");
		if((writen(fd , "KO errore readn \n", 24)) == -1){
			perror("Errore nella write4");
			inc_numero_anomalie_client();		
			return 0;
		}	
		free(buffer_file);		
		close(fd_file);
		return 0;
	}
	close(fd_file);
	//DATA len \n data
	int dimensione_toSend = size_file +1 +strlen(buffer_file)+1;
	char *toSend =calloc(dimensione_toSend , sizeof(char));
	if(toSend == NULL){
		perror("Errore calloc myRetrieve");
		free(buffer_file);
		free(tmp);
		if((writen(fd , "KO errore calloc \n", 18)) == -1){
			perror("Errore nella write3");
			inc_numero_anomalie_client();		
			return 0;
		}
		return 0;
	}
	memset(toSend , 0 , dimensione_toSend);
	strcpy(toSend , "DATA ");
	strcat(toSend , tmp);
	strcat(toSend , " \n ");
	strcat(toSend , buffer_file);
	if(VERBOSE)
		printf("Lunghezza del dato effettivo %lld , lunghezza file letta %ld\n",size_file,strlen(buffer_file));
	if(writen(fd , toSend , strlen(toSend)) < 0){
		if (errno == EINTR){
			errno = 0;
			writen(fd , toSend , strlen(toSend));
		}
		perror("Errore nella write4");
		inc_numero_anomalie_client();
		free(buffer_file);
		free(toSend);
		free(tmp);
		return 0;
	}
	if(VERBOSE)
		printf("OK ");
	free(buffer_file);
	free(toSend);
	free(tmp);
	return 1;
}

/*
	Esegue la disconnesione del client nome
*/
int myLeave(int fd ,char *nome){
	char *cmd = calloc(8 , sizeof(char));
	if((read(fd , cmd , 8)) < 0){
		perror("Errore nella read myLeave");
		inc_numero_anomalie_client();
		free(cmd);
		close(fd);
		return 0;						
	}
	if(strcmp(cmd,"EAVE \n")==0){
		if((writen(fd , "OK \n", 4)) == -1){
			perror("Errore nella write1 myLeave");
			inc_numero_anomalie_client();
			close(fd);
			free(cmd);		
			return 0;
		}
	}
	else{
		if((writen(fd , "KO comando non riconosciuto \n", 29)) == -1){
			perror("Errore nella write1 myLeave");
			close(fd);
			free(cmd);
			return 0;
		}
	}
	time_t tick;
	tick=time(NULL);
	if(VERBOSE){
		printf("Client %s si disconnette in data %s\n", nome,ctime(&tick));
		printf("LEAVE\n");
	}

	close(fd);
	free(cmd);
	return 0;
}

/*---------------------------------------------------Metodi per le variabili globali-----------------------------------------------------------*/

void inc_client_totali() {
	pthread_mutex_lock(&client_totali_mutex);
	client_totali++;
	pthread_mutex_unlock(&client_totali_mutex);
}

void inc_stored_data(int size){
	pthread_mutex_lock(&stored_Data_mutex);
	stored_Data += size;
	pthread_mutex_unlock(&stored_Data_mutex);
}

void dec_stored_data(int size){
	pthread_mutex_lock(&stored_Data_mutex);
	stored_Data -= size;
	pthread_mutex_unlock(&stored_Data_mutex);
}

void inc_numero_nuovi_file(){
	pthread_mutex_lock(&numero_file_mutex);
	numero_file++;
	pthread_mutex_unlock(&numero_file_mutex);
}
void inc_numero_anomalie_client(){
	pthread_mutex_lock(&anomalie_client_mutex);
	numero_anomalie_client++;
	pthread_mutex_unlock(&anomalie_client_mutex);
}
void inc_numero_fileRimossi(){
	pthread_mutex_lock(&numero_fileRimossi_mutex);
	numero_fileRimossi++;
	pthread_mutex_unlock(&numero_fileRimossi_mutex);
}
void inc_client_connessi(){
	pthread_mutex_lock(&client_connessi_mutex);
	if(client_connessi > CONNECT_MAX){
		while(client_connessi <= CONNECT_MIN){
			pthread_cond_wait(&controllo , &client_connessi_mutex);
		}
	}
	client_connessi++;
	pthread_mutex_unlock(&client_connessi_mutex);
}

void dec_client_connessi(){
	pthread_mutex_lock(&client_connessi_mutex);
	client_connessi--;
	pthread_cond_signal(&controllo);
	pthread_mutex_unlock(&client_connessi_mutex);
}

int readn(long fd, void *buf, size_t size) {
	size_t left = size;
	int r;
	char *bufptr = (char*)buf;
	while(left>0) {
		if ((r=read((int)fd ,bufptr,left)) == -1) {
			if (errno == EINTR) continue;
			return -1;
		}
		if (r == 0) return 0;   // fine messaggio
		left    -= r;
		bufptr  += r;
	}
	return size;
}