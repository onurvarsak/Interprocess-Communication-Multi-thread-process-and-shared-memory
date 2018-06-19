#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdio.h>


#define MAXELEMENTS 5
#define THREADSIZE 5
#define MAXLENGT 64
#define QUEUESIZE 3

struct Queue{
    int front, rear, size;
    char **lines;
};


struct Queue* createQueue(int);

void doPermutation(char *, char *, int );   /* does permutation operation           */
void enqueue(struct Queue *, char *);       /* does add queue element to end queue  */
void line_copy(char *, char *);             /* copies two string                    */
void subbox(char *, char *);                /* does sub box operation               */
void *xor(char *, char *);                  /* does xor operation                   */
void read_key(char *);

void create_shared_memory(char *);          /* shared memory operation              */

char *dequeue(struct Queue *);              /* get first add element from queue     */

int get_subbox_val(int val);                /* get element sub box array            */
int isEmpty(struct Queue *);                /* is empty or not to queue             */
int isFull(struct Queue *);                 /* is full or not to queue              */

/* these functions are used to by threads */
void* read_plain_text_put_plain_queue();
void* read_plain_queue_xor_put_xor_queue();
void* read_xor_queue_permutation_put_permutation_queue();
void* read_perm_queue_authentication_aut_queue();
void* get_subtext_send_proc2();

/* pipe and message send and receive fonction*/
void createPipe(int *p);
void sendPipe(char *string, int *pipe);
void convertToChar(char *p0, char *p1, int *pipe1);

char *key;
pthread_mutex_t mutex;


struct Queue* plain_queue;
struct Queue* xor_queue;
struct Queue* perm_queue;
struct Queue* aut_queue;

int finish_thread_read_file = 0;
int finish_thread_xor = 0;
int finish_thread_permutation = 0;
int finish_thread_authentication = 0;

char *plain_name;
char *key_name;

int main(int argc, char **argv){

    plain_name = argv[1];
    key_name = argv[2];

    /* create all threads */
    pthread_t *thread_read_file         = (pthread_t*) malloc(sizeof(pthread_t));
    pthread_t *thread_xor               = (pthread_t*) malloc(sizeof(pthread_t));
    pthread_t *thread_permutation       = (pthread_t*) malloc(sizeof(pthread_t));
    pthread_t *thread_authentication    = (pthread_t*) malloc(sizeof(pthread_t));
    pthread_t *send_proc2               = (pthread_t*) malloc(sizeof(pthread_t));

    /* create all queue  */
    plain_queue     = createQueue(MAXELEMENTS);
    xor_queue       = createQueue(MAXELEMENTS);
    perm_queue      = createQueue(MAXELEMENTS);
    aut_queue       = createQueue(MAXELEMENTS);

    key = (char *)malloc(sizeof(char) * MAXLENGT);
    read_key(key_name);

    pthread_mutex_init(&mutex, NULL);

    /* running all threads */
    pthread_create(&thread_read_file, (pthread_attr_t*) NULL, read_plain_text_put_plain_queue, NULL);
    pthread_create(&thread_xor, (pthread_attr_t*) NULL, read_plain_queue_xor_put_xor_queue, NULL);
    pthread_create(&thread_permutation, (pthread_attr_t*) NULL, read_xor_queue_permutation_put_permutation_queue, NULL);
    pthread_create(&thread_authentication, (pthread_attr_t*) NULL, read_perm_queue_authentication_aut_queue, NULL);
    pthread_create(&send_proc2, (pthread_attr_t*) NULL, get_subtext_send_proc2, NULL);
    /* wait finish all threads */
    pthread_join(thread_read_file, NULL);
    pthread_join(thread_xor, NULL);
    pthread_join(thread_permutation, NULL);
    pthread_join(thread_authentication, NULL);
    pthread_join(send_proc2, NULL);

    pthread_mutex_destroy(&mutex);
    return 0;
}


void create_shared_memory(char *line){
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

    str = line;
    flag = "true";

    shmdt(str);
    shmdt(flag);
}

void* get_subtext_send_proc2(){
    int     processId   = 0;
    int     pipe_message[2];
    char    p0[10], p1[10];

    createPipe(pipe_message);
    convertToChar(p0, p1, pipe_message);

    processId = fork();

    for(;;){
        switch(processId) {
           case -1:
               printf("Fork Error");
               break;
           case 0:
                execv("./process-2", ((char *[]){ "./process-1", p0 , p1, NULL }));
                exit(1);
                break;
            default:
                while(1){
                    while(1){
                        pthread_mutex_lock(&mutex);
                        if((finish_thread_authentication == 1) && (aut_queue->size == 0)){
                            sendPipe("end", pipe_message);
                            pthread_mutex_unlock(&mutex);
                            return NULL;
                        }
                        else{
                            pthread_mutex_unlock(&mutex);
                            break;
                        }
                    }
                    while(1){
                        pthread_mutex_lock(&mutex);
                        if(aut_queue->size == 0){
                            pthread_mutex_unlock(&mutex);
                        }
                        else{
                            pthread_mutex_unlock(&mutex);
                            break;
                        }
                    }
                    pthread_mutex_lock(&mutex);
                    char *line = dequeue(aut_queue);
                    create_shared_memory(line);
                    sendPipe(line, pipe_message);
                    pthread_mutex_unlock(&mutex);
                }
                wait(NULL);
        }
    }

    return NULL;

}

void createPipe(int *p){
    pipe(p);
}
void sendPipe(char *string, int *pipe){
    write(pipe[1], string, MAXLENGT);
}
void convertToChar(char *p0, char *p1, int *pipe1){
	sprintf(p0, "%d", *(pipe1 + 0));
	sprintf(p1, "%d", *(pipe1 + 1));
}

void* read_plain_text_put_plain_queue(){
    FILE *plainFile = fopen(plain_name, "r");
    char *line = (char *) malloc(sizeof(char) * MAXLENGT);
    char ch;
    int i = 0;

    while((ch = fgetc(plainFile)) != EOF){
        if(ch == '\n'){
            line[i] = '\0';
            while(1){
                pthread_mutex_lock(&mutex);
                if(plain_queue->size == MAXELEMENTS){
                    pthread_mutex_unlock(&mutex);
                }
                else{
                    pthread_mutex_unlock(&mutex);
                    break;
                }
            }
            pthread_mutex_lock(&mutex);
            enqueue(plain_queue, line);
            pthread_mutex_unlock(&mutex);
            line = (char *) malloc(sizeof(char) * MAXLENGT);
            i = 0;
        }
        else
            line[i++] = ch;
    }
    finish_thread_read_file = 1;
    return NULL;
}
void* read_plain_queue_xor_put_xor_queue(){
    FILE *fptr = fopen("thread_2-xor.txt", "w");

    while(1){
        while(1){
            pthread_mutex_lock(&mutex);
            if((finish_thread_read_file == 1) && (plain_queue->size == 0)){
                finish_thread_xor = 1;
                pthread_mutex_unlock(&mutex);
                return NULL;
            }
            else{
                pthread_mutex_unlock(&mutex);
                break;
            }
        }
        while(1){
            pthread_mutex_lock(&mutex);
            if(plain_queue->size == 0){
                /** queue bossa burada donmesi lazým thread'in dolu oldugunda iþlemini yapmalý*/
                pthread_mutex_unlock(&mutex);
            }
            else{
                pthread_mutex_unlock(&mutex);
                break;
            }
        }

        pthread_mutex_lock(&mutex);
        char *line = dequeue(plain_queue);
        pthread_mutex_unlock(&mutex);

        char *xor_line = (char *)malloc(sizeof(char) * MAXLENGT);
        xor(line, xor_line);
        /** 2.queue ye xor'lanmis line ý ekliyoruz */
        /** 2.queue doluysa beklememiz lazým */
        while(1){
            pthread_mutex_lock(&mutex);
            if(xor_queue->size == MAXELEMENTS){
                pthread_mutex_unlock(&mutex);
            }
            else{
                pthread_mutex_unlock(&mutex);
                break;
            }
        }
        pthread_mutex_lock(&mutex);
        enqueue(xor_queue, xor_line);
        fprintf(fptr,"%s\n", xor_line);
        pthread_mutex_unlock(&mutex);
   }
   fclose(fptr);
   return NULL;
}
void* read_xor_queue_permutation_put_permutation_queue(){
    FILE *fptr = fopen("thread_3-perm.txt", "w");
    while(1){
        while(1){
            pthread_mutex_lock(&mutex);
            if((finish_thread_xor == 1) && (xor_queue->size == 0)){
                finish_thread_permutation = 1;
                pthread_mutex_unlock(&mutex);
                return NULL;
            }
            else{
                pthread_mutex_unlock(&mutex);
                break;
            }
        }
        while(1){
            pthread_mutex_lock(&mutex);
            if(xor_queue->size == 0){
                pthread_mutex_unlock(&mutex);
            }
            else{
                pthread_mutex_unlock(&mutex);
                break;
            }
        }
        pthread_mutex_lock(&mutex);
        char *line = dequeue(xor_queue);
        pthread_mutex_unlock(&mutex);

        char *permutation = (char *)malloc(sizeof(char) * MAXLENGT);
        doPermutation(permutation, line, strlen(line) + 1);

        /** 3.thread permutation iþleminden sonra queues[1] e atacaðý deðeri eðer queues[1] doluysa bekleyecek boþalmasýný*/
        while(1){
            pthread_mutex_lock(&mutex);
            if(perm_queue->size == MAXELEMENTS){
                pthread_mutex_unlock(&mutex);
            }
            else{
                pthread_mutex_unlock(&mutex);
                break;
            }
        }
        pthread_mutex_lock(&mutex);
        enqueue(perm_queue, permutation);
        fprintf(fptr,"%s\n", permutation);
        pthread_mutex_unlock(&mutex);
    }
    fclose(fptr);
    return NULL;
}
void* read_perm_queue_authentication_aut_queue(){
    FILE *fptr = fopen("thread_4-subbox.txt", "w");
    while(1){
        while(1){
            pthread_mutex_lock(&mutex);
            if((finish_thread_permutation == 1) && (perm_queue->size == 0)){
                finish_thread_authentication = 1;
                pthread_mutex_unlock(&mutex);
                return NULL;
            }
            else{
                pthread_mutex_unlock(&mutex);
                break;
            }
        }

        while(1){
            pthread_mutex_lock(&mutex);
            if(perm_queue->size == 0){
                pthread_mutex_unlock(&mutex);
            }
            else{
                pthread_mutex_unlock(&mutex);
                break;
            }
        }
        pthread_mutex_lock(&mutex);
        char *line = dequeue(perm_queue);
        pthread_mutex_unlock(&mutex);

        char *subb = (char *)malloc(sizeof(char) * MAXLENGT);
        subbox(line, subb);
        while(1){
            pthread_mutex_lock(&mutex);
            if(aut_queue->size == MAXELEMENTS){
                pthread_mutex_unlock(&mutex);
            }
            else{
                pthread_mutex_unlock(&mutex);
                break;
            }
        }
        pthread_mutex_lock(&mutex);
        enqueue(aut_queue, subb);
        fprintf(fptr,"%s\n", subb);
        pthread_mutex_unlock(&mutex);
    }
    fclose(fptr);
    return NULL;
}
void enqueue(struct Queue *queue, char *line){
    if (isFull(queue))      return;

    queue->rear = (queue->rear + 1) % MAXELEMENTS;
    queue->size = queue->size + 1;
    queue->lines[queue->rear] = line;
}
char *dequeue(struct Queue *queue){
    if (isEmpty(queue))     return NULL;

    char *line = (char *)malloc(sizeof(char) * MAXLENGT);
    line_copy(queue->lines[queue->front], line);
    queue->lines[queue->front] = NULL;
    queue->front    = (queue->front + 1) % MAXELEMENTS;
    queue->size     = queue->size - 1;

    return line;
}
struct Queue* createQueue(int element_size){
    struct Queue* queue = (struct Queue*) malloc(sizeof(struct Queue));
    int i = 0;

    queue->size     = 0;
    queue->front    = 0;
    queue->rear     = element_size - 1;
    queue->lines    = (char **)malloc(sizeof(char *) * element_size);

    for(; i < element_size; i++)
        queue->lines[i] = (char *)malloc(sizeof(char) * MAXLENGT);

    return queue;
}
int isFull(struct Queue* queue){
    return (queue->size == MAXELEMENTS);
}
int isEmpty(struct Queue* queue){
    return (queue->size == 0);
}
void read_key(char *file_name){
    FILE* key_file = fopen(file_name, "r");
    int index = 0;
    char ch;
    while((ch = fgetc(key_file)) != '\n'){
        key[index] = ch;
        index = index + 1;
    }
    key[index] = '\0';
}
void line_copy(char *queue_line, char *line){
    int i = 0;
    while(queue_line[i] != '\0'){
        line[i] = queue_line[i];
        i = i + 1;
    }
    line[i] = '\0';
}
void *xor(char *str, char *line){
    int i = 0;
    int j = 0;
    int k = 0;
    int size = 0;

    int num_1 = 0;
    int num_2 = 0;
    int xor_result = 0;

    char *string = NULL;

    for(;;){
        if(size == 16)
            break;

        string = malloc(sizeof(char) * 4);

        for(;;){
            if((key[i] == '-') || (key[i] == '\0')){
                i++;
                break;
            }
            string[k++] = key[i++];
        }

        string[k] = '\0';
        k = 0;
        num_1 = atoi(string);
        string = malloc(sizeof(char) * 4);


        for(;;){
            if((str[j] == '-') || (str[j] == '\0')){
                j++;
                break;
            }
            string[k++] = str[j++];
        }
        string[k] = '\0';
        k = 0;

        num_2 = atoi(string);
        xor_result = num_1 ^ num_2;

        string = malloc(sizeof(char) * 4);

        sprintf(string, "%d", xor_result);
		if(size < 15){
            strcat(line, string);
            strcat(line, "-");
		}
		else{
            strcat(line, string);
            strcat(line, "\0");
		}
        size++;
    }
}
void doPermutation(char *permutation, char *line, int length ){

    char *first = malloc(sizeof(char) * 32);
    char *second = malloc(sizeof(char) * 32);
    int size = 0, i = 0, j = 0, k = 0;

    while(1){
        if(line[k] == '\0')
            break;

        if(line[k] == '-'){
            size++;
            if(size == 8){
                k++;
                continue;
            }
        }
        if(size < 8)
            first[i++] = line[k++];
        else
            second[j++] = line[k++];
    }

    first[i]    = '\0';
    second[i]   = '\0';
    i = 0;j = 0;k = 0;

    while(second[i] != '\0')
        permutation[k++] = second[i++];

    permutation[k++] = '-';

    while(first[j] != '\0')
        permutation[k++] = first[j++];

    permutation[k] = '\0';
}
void subbox(char *permu, char *subb){
    char *sub = malloc(sizeof(char) * 4);
    int index = 0;
    int size = 0;
    int i = 0;
    int j = 0;

    for(;;){
        if((permu[i] == '-') || (permu[i] == '\0')){
            size++;
            sub[j] = '\0';
            j = 0;
            index = atoi(sub);
            sub = malloc(sizeof(char) * 4);
            sprintf(sub, "%d", get_subbox_val(index));

            if(size == 16){
                strcat(subb, sub);
                strcat(subb, "\0");
            }
            else{
                strcat(subb, sub);
                strcat(subb, "-");
            }
            if(permu[i] == '\0')
                break;
            i++;
        }
        else{
            sub[j++] = permu[i++];
        }
    }
}
int get_subbox_val(int val){
    int SUBBOX[256] = {47, 164, 147, 166, 221, 246, 1, 13, 198, 78, 102, 219, 75, 97, 62, 140,
			     84, 69, 107, 99, 185, 220, 179, 61, 187, 0, 92, 112, 8, 33, 15, 119,
			     209, 178, 192, 12, 121, 239, 117, 96, 100, 126, 118, 199, 208, 50, 42, 168,
			     14, 171, 17, 238, 158, 207, 144, 58, 127, 182, 146, 71, 68, 157, 154, 88,
			     248, 105, 131, 235, 98, 170, 22, 160, 181, 4, 254, 70, 202, 225, 67, 205,
			     216, 25, 43, 222, 236, 128, 122, 77, 59, 145, 167, 54, 20, 55, 152, 149,
			     230, 211, 224, 111, 165, 124, 16, 243, 213, 114, 116, 63, 64, 176, 31, 161,
			     9, 229, 95, 247, 193, 18, 134, 79, 133, 173, 82, 51, 57, 136, 6, 49,
			     5, 197, 115, 65, 169, 255, 249, 195, 30, 162, 150, 53, 83, 46, 228, 81,
			     237, 104, 28, 223, 217, 251, 200, 60, 132, 194, 151, 137, 191, 74, 201, 103,
			     29, 80, 113, 101, 250, 172, 234, 180, 73, 141, 204, 27, 241, 188, 153, 155,
			     86, 94, 177, 87, 39, 91, 2, 48, 35, 40, 120, 159, 184, 123, 215, 138,
			     210, 108, 76, 106, 36, 189, 125, 226, 252, 37, 66, 156, 253, 218, 85, 203,
			     110, 10, 244, 45, 34, 242, 72, 93, 52, 135, 44, 245, 3, 32, 196, 163,
			     232, 240, 227, 24, 139, 183, 38, 233, 130, 143, 109, 41, 174, 231, 129, 23,
			     148, 89, 212, 19, 21, 142, 7, 214, 56, 90, 11, 190, 175, 206, 26, 186
    };
    unsigned char sub_box_val = SUBBOX[val];
    return sub_box_val;
}
