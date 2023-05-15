#include "utils_client.h"
#define BASE_STRINGA "0101010101"
int failure;

int test1(){
	int i, j;
	size_t len = 100;
	char * blocco, *pointer;
	char name[20]; 
	for(i = 1 ; i <= 20 ; i++){
		blocco =  calloc(len, sizeof(char));
		if(blocco == NULL){
			perror("Errore nella calloc");
			return 0;
		}
		sprintf(name, "File_%d.txt", i);
		pointer = blocco;
		for(j = 0; j < len; j+=10){
			strncpy(pointer, BASE_STRINGA, 10);
			//sposto il pointer di 10 posizioni in avanti
			pointer += 10;
		}
		if(!os_store(name, (void*) blocco, len)) failure++;
		len += 5260;
		free(blocco);
	}
	return 0;
}

int test2(){
	int i, j;
	size_t len = 100;
	char * blocco, *pointer, *risultato;
	char name[20];
	for(i = 1 ; i <= 20 ; i++){
		blocco =  calloc(len, sizeof(char));
		if(blocco == NULL){
			perror("Errore nella calloc");
			return 0;
		}
		sprintf(name, "File_%d.txt", i);
		pointer = blocco;
		for(j = 0; j < len; j+=10){
			strncpy(pointer, BASE_STRINGA, 10);
			pointer += 10;
		}
		risultato = os_retrieve(name);
		if((risultato == NULL) || (strncmp(risultato, blocco , len) != 0)){
			free(blocco);
			 return 0;
		}
		else			printf("File_%d integro\n",i);
		free(risultato);
		free(blocco);
		len +=5260;
	}
	return 1;
}


int test3(){
	int i;
	size_t len = 100;
	char name[20]; 
	for(i = 1 ; i <= 20 ; i++){
		sprintf(name, "File_%d.txt", i);
		if(!os_delete(name)) failure++;
		len += 5260;
	}
	return 0;
}

int main(int argc, char *argv[]) {

	failure = 0;
	if(argc != 3) return 0;
	if (os_connect(argv[1])) {
		switch (atol(argv[2])) {
			case 1:
				test1();
				printf("%s Failure Test1: %d su 20\n",argv[1], failure);
				break;
			case 2:
				if(test2() == 0)			failure++;
				printf("%s Failure Test2: %d su 20\n", argv[1],failure);
				break;
			case 3:
				test3();
				printf("%s Failure Test3: %d su 20\n",argv[1], failure);
				break;
			default:
				break;
		}
	}
	else 
		return 1;
	os_disconnect();
	return 1;
	
}
