#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>


#define MAXLENGT 64

int blocks[16] = {0};

/* pipe and message send and receive fonction*/
void convertToInt(char *p0, char *p1, int *p);
void getPipe(char *string, int *p);
void calculate_sum(char *line);
void createPipe(int *p);

/* shared memory operation */
char* create_shared_memory();


int main(int argc, char **argv)
{
    FILE *f = fopen("process-2-final.txt", "w");
    int i = 0;
    int pipe[2];
    createPipe(pipe);
    convertToInt(argv[1], argv[2], pipe);
    for(;;){
        char *line = (char *)malloc(sizeof(char) * MAXLENGT);
        getPipe(line, pipe);
        char *shared_line = create_shared_memory();
        if(strcmp(line, "end") == 0)
            break;
        calculate_sum(line);
        int j;
        for(j = 0; j < 16; j++)
            if(j != 15)
                fprintf(f,"%d-", blocks[j]);
            else
                fprintf(f,"%d\n", blocks[j]);


    }
    fclose(f);
}


char* create_shared_memory(){
    key_t key_message;
    key_t key_flag;

    if ((key_message = ftok("/tmp", 'a')) == (key_t) -1) {
        perror("IPC error: ftok");
    }
    if ((key_flag = ftok("/home", 'a')) == (key_t) -1) {
        perror("IPC error: ftok");
    }

    int key_shmid = shmget(key_message,1024,0666|IPC_CREAT);
    int flag_shmid = shmget(key_flag,1024,0666|IPC_CREAT);

    char *str = (char*)shmat(key_shmid, (void*)0, 0);
    char *flag = (char*)shmat(flag_shmid, (void*)0, 0);

    flag = "false";

    shmdt(str);
    shmdt(flag);
    return str;
}

void calculate_sum(char *line){

    int i = 0;
    int j = 0;
    int k = 0;

    char *block = (char *)malloc(sizeof(char) * 4);

    while(1){
        if(line[i] == '-'){
            block[j] = '\0';
            blocks[k] = blocks[k] + atoi(block);
            if(blocks[k] > 255)
                blocks[k] = (blocks[k] % 256) + 1;

            block = (char *)malloc(sizeof(char) * 3);
            k++;
            i++;
            j = 0;
        }
        else if(line[i] == '\0'){
            block[j] = '\0';
            blocks[k] = blocks[k] + atoi(block);
            if(blocks[k] > 255)
                blocks[k] = (blocks[k] % 256) + 1;
            break;
        }
        else{
            block[j++] = line[i++];
        }
    }
}


void getPipe(char *string, int *p){
    read(p[0], string, MAXLENGT);
}

void createPipe(int *p){
    pipe(p);
}

void convertToInt(char *p0, char *p1, int *p){
	p[0] = atoi(p0);
	p[1] = atoi(p1);
}
