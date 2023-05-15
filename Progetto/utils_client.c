#include "utils_client.h"
#define BLOCCO_DATI 1024
int fd_skt;

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
			perror("Errore nella read myRead");
			return NULL;
		}
		lim = bytetotali;
		bytetotali += byteletti;
		i = lim;
		while(i < bytetotali && !trovato){
			if(buffer[i] == '\n') trovato = 1;
			i++;
		}
		message = realloc(message , bytetotali * sizeof(char)+1);
		strncat(message , buffer , byteletti);
		buffer = memset(buffer, 0, BLOCCO_DATI);
	}
	free(buffer);
	return message;

}

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

int os_connect(char * name){
	if(name == NULL){
		fprintf(stderr, "Nome passato non valido : os_connect");
		return 0;
	}
	int len_name = strlen(name);
	if(MAX_SIZE_FILE < len_name){
		perror("Nome non valido MAX_SIZE_FILE : os_connect");
		return 0;
	}
	struct sockaddr_un sa;
	strncpy(sa.sun_path, SOCKNAME ,UNIX_PATH_MAX);
	sa.sun_family=AF_UNIX;
	fd_skt = socket(AF_UNIX,SOCK_STREAM,0);
	while (connect(fd_skt,(struct sockaddr*)&sa, sizeof(sa)) == -1 ) {
		if ( errno == ENOENT ) sleep(1); /* sock non esiste */
		else exit(EXIT_FAILURE); 
	}
	int toSend_len = 10+ len_name +2; 	//Lungezza stringa considerando "Register " + Lunghezza nel nome in input + '\n'.
	char *toSend = calloc(toSend_len,sizeof(char));
	if(toSend == NULL){
		perror("Malloc non riuscita");
		return 0;
	}
	strncpy(toSend , "REGISTER ",10);
	strcat(toSend , name);
	strcat(toSend , " \n");
	char *risposta = calloc(RESPONSE_SIZE , sizeof(char));
	if((writen(fd_skt , toSend , (toSend_len - 1)) == -1)){
		perror("Errore nella writen");
		return 0;
	}
	if((read(fd_skt , risposta , RESPONSE_SIZE) == -1)){
		perror("Errore nella lettura : os_connect");
		exit(EXIT_FAILURE);
	}
	if(risposta == NULL){
		perror("Risposta non valida");
		exit(EXIT_FAILURE);	
	}
	int esito = 0;
	esito = strcmp(risposta , "OK \n");

	free(risposta);
	free(toSend);

	if(esito == 0){
		printf("MI REGISTRO\n");
		return 1;
	}
	else{
		close(fd_skt);
		return 0;
	}
}

void * os_retrieve(char * name){
	if(fd_skt < 0)	return NULL;
	if(name == NULL) return NULL;
	int name_len = strlen(name);
	int msg_len;
	int lunghezza_finale = 0;
	char *msg;
	char *block;

	msg_len = name_len + 12; //12 char necessari per scrivere 'RETRIEVE name \n\0'
	if(((msg = calloc(msg_len , sizeof(char))) == NULL)){
		perror("Errore nella calloc");
		return NULL;
	}
	strncpy(msg, "RETRIEVE ", 9);
	strcat(msg, name);
	strcat(msg, " \n");
	printf("RETRIEVE %s\n",name);
	if(writen(fd_skt , msg , msg_len) < 0){
		free(msg);
		perror("Errore nella writen");
		return NULL;
	}
	free(msg);
	//Utilizzo my read perchè la risposta potrebbe essere Data len \n dato
	char *risposta = myRead(fd_skt);
	if(risposta == NULL){		
		perror("Errore nella myRead");
		return NULL;
	}
	if(strncmp(risposta, "KO ", 3) == 0){
		printf("%s",risposta);
		free(risposta);		
		return NULL;
	}
	if(strncmp(risposta, "DATA ", 5) != 0){
		perror("comando non conosciuto");
		free(risposta);		
		return NULL;
	}
	int numeroTok = 0;
	char **tok = tokenizer(risposta ,&numeroTok);
	free(risposta);
	if(numeroTok < 2){
		perror("Errore tokenizer");
		free(tok[0]);
		free(tok);
		return NULL;
	}
	//Ottengo la lunghezza
	lunghezza_finale = atoi(tok[1]);
	if(lunghezza_finale <= 0){
		return NULL;
	}
	block = tok[2];
	if(block == NULL){
		perror("Errore in strtok");
		return NULL;
	}
	int dif = lunghezza_finale - strlen(block);
//	Stesso ragionamento della myStore lato server.
	char *finale;
	if(dif == 0){
		for(int i = 0 ; i < numeroTok ; i++){
			if(strcmp(block ,tok[i]) != 0){
				free(tok[i]);
			}
		}
		free(tok);
		return block;
	}
	else{
		int val = 0;
		finale = calloc(lunghezza_finale + 1, sizeof(void));
		if(finale == NULL){
			perror("Not enough memory");
			return NULL;
		}
		strcpy(finale , block);
		free(block);
		char *read_more = calloc(dif+1 , sizeof(void));
		if(read_more == NULL){
			for(int i = 0 ; i < numeroTok ; i++){
				free(tok[i]);
			}
			free(tok);
			free(finale);
			perror("Errore nella calloc");
			return NULL;
		}
		memset(read_more , 0 , dif+1);
		if((val = readn(fd_skt , read_more , dif)) < 0){
			for(int i = 0 ; i < numeroTok ; i++){
				free(tok[i]);
			}
			free(tok);
			free(read_more);
			free(finale);
			return NULL;
		}
		printf("Ho finito di leggere\nval == %d , dif == %d , lunghezza_finale == %d , Lunghezza del blocco == %ld\n",val , dif , lunghezza_finale , strlen(finale));
		strcat(finale , read_more);
		free(read_more);
		if(lunghezza_finale != strlen(finale)){
			printf("Dati letti parzialmente , errore  readn\n");
			return NULL;
		}
	}
	free(tok[0]);
	free(tok[1]);

	free(tok);
	return finale;
}





int os_store(char * name, void * blo, size_t len){
	if(fd_skt < 0)	return 0;
	if(name == NULL || blo == NULL || len == 0) {
		perror("Errore nella os_store");
		return 0;
	}
	long len_name = strlen(name)+1;			// "STORE name len \n block"
	long toSend_len = 16 + len_name + len +1;	// 14 -> 7 sono per "STORE " altri 7 perchi al massimo block al massimo sarà 100000 + 1 spazio
	char *toSend = calloc(toSend_len , sizeof(char));
	if(toSend == NULL){
		perror("Errore nella calloc1 os_store");
		return 0;
	}
	strcpy(toSend , "STORE ");
	strcat(toSend , name);
	strcat(toSend , " ");
	char tmp[12];
	if(tmp == NULL){
		perror("Errore nella calloc os_store");
		return 0;
	}
	sprintf(tmp , "%ld", len);
	strcat(toSend , tmp);
	strcat(toSend , " \n ");
	strncat(toSend , blo ,len);
	if((writen(fd_skt , toSend , strlen(toSend)+1) < 0)){
		perror("Errore nella writen os_store");
		free(toSend);
		return 0;
	}
	free(toSend);
	char *risposta = calloc(RESPONSE_SIZE , sizeof(char));
	if(risposta == NULL){
		perror("Errore nella calloc os_store");
		return 0;
	}
	read(fd_skt , risposta , RESPONSE_SIZE);
	if(!strcmp(risposta , "OK \n")){
		printf("STORE %s\n",risposta);
		free(risposta);
		return 1;
	}
	else{
		printf("STORE %s\n",risposta);
		free(risposta);
		return 0;
	}
	return 1;
}



int os_delete(char * name){
	if(fd_skt < 0)	return 0;
	int name_len = strlen(name)+1;
	char *toSend = calloc(name_len + 9 ,sizeof(char));
	toSend = memset(toSend , 0 ,name_len +9);
	strcpy(toSend,"DELETE ");
	strcat(toSend , name);
	strcat(toSend , " \n");
	if((writen(fd_skt , toSend , strlen(toSend)+1) < 0)){
		perror("Errore nella writen os_store");
		free(toSend);
		return 0;
	}
	free(toSend);
	char *risposta = calloc(RESPONSE_SIZE , sizeof(char));
	risposta = memset(risposta , 0 , RESPONSE_SIZE);
	if(risposta == NULL){
		perror("Errore nella calloc os_store");
		return 0;
	}
	read(fd_skt , risposta , RESPONSE_SIZE);
	if(strcmp(risposta , "OK \n") == 0){
		printf("DELETE %s\n",risposta);
		free(risposta);
		return 1;
	}
	else{
		printf("DELETE %s\n",risposta);
		free(risposta);
		return 0;
	}	
}


int os_disconnect(){
	if(fd_skt < 0)	return 0;
	if(writen(fd_skt , "LEAVE \n" , 8) < 0){
		perror("Errore nella writen1 : os_disconnect");
		return 0;
	}
	char *buff = calloc(RESPONSE_SIZE , sizeof(char));
	if(buff == NULL){
		perror("Errore nella calloc1 : os_disconnect");
		return 0;
	}
	read(fd_skt , buff , RESPONSE_SIZE);
	if(!strcmp(buff , "OK \n")){
		printf("Disconnessione con successo\n");
		free(buff);
		close(fd_skt);
		return 1;
	}
	else{
		printf("Disconnessione fallita\n");
		free(buff);
		close(fd_skt);
		return 0;
	}
}
