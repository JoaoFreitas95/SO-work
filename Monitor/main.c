// SERVER

#include <stdio.h> 
#include <netdb.h>
#include <netinet/in.h> 
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h> 
#include <sys/types.h> 
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "configuration/parser.c"
#define SA struct sockaddr 
#define MAX_SOCKET_LENGTH 1024

struct monitorThreadInfo {
    int sockfd;
};

struct statsInfo {
    int numUsers;

    int numUsersInTestingQueue;
    int numUsersDoingTheTest;
    int numUsersDoneTheTest;
    int numUsersFastTest;
    int numUsersNormalTest;
    int numUsersIsolated;
    int numUsersIsolatedInHome;
    int numNegativeTests;
    int numPositiveTests;
    int numDeaths;
    int numHealed;
    int numHospitalized;

    // Averages params
    int totalUsersPassedInQueue;
    int totalSecondsPassedInQueue;

    int totalUsersDoneTests;
    int totalSecondsDoingTests;

    int totalUsersWaitedTestResult;
    int totalSecondsUntilTestResult;

    int totalUsersIsolated;
    int totalSecondsIsolated;

    int isRunning;
};

struct statsInfo sInfo;

char LOG_FILE_NAME[] = "log.txt";

int getSocket(struct Configuration config){
    int sockfd, connfd, len; 
    struct sockaddr_in servaddr, cli; 
  
    // socket create and verification 
    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd == -1) { 
        printf("socket creation failed...\n"); 
        exit(0); 
    } 
    else{
        printf("Socket successfully created..\n"); 
    }

    bzero(&servaddr, sizeof(servaddr)); 
  
    // assign IP, PORT 
    servaddr.sin_family = AF_INET; 
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    servaddr.sin_port = htons(config.listenPort); 
  
    // Binding newly created socket to given IP and verification 
    if ((bind(sockfd, (SA*)&servaddr, sizeof(servaddr))) != 0) { 
        printf("socket bind failed...\n"); 
        exit(0); 
    } 
    else{
        printf("Socket successfully binded..\n"); 
    }
  
    // Now server is ready to listen and verification 
    if ((listen(sockfd, 1)) != 0) { 
        printf("Listen failed...\n"); 
        exit(0); 
    } 
    else{
        printf("Server listening on the port %d..\n", config.listenPort); 
    }
    len = sizeof(cli); 
  
    // Accept the data packet from client and verification 
    connfd = accept(sockfd, (SA*)&cli, &len); 
    if (connfd <= 0) { 
        printf("server acccept failed...\n"); 
        exit(0); 
    } 
    else{
        printf("server acccept the client. (Response: %d)...\n", connfd); 
    }
    sInfo.isRunning = 1;
  
    return connfd;
}

void emptyLogFile(){
    FILE *fp;
    fp = fopen(LOG_FILE_NAME, "w+");
    fprintf(fp, "");
    fclose(fp);
}

void appendToLogFile(char buff[]){
    FILE *fp;
    fp = fopen(LOG_FILE_NAME, "a+");
    fprintf(fp, strcat(buff, "\n"));
    fclose(fp);
}

void parseResponse(char buff[]){
    char eventDelim[] = "-";
    char paramsDelim[] = ",";

    char *event = strtok(buff, eventDelim);
    char *params = strtok(NULL, eventDelim);
    char *firstParam = strtok(params, paramsDelim);
    char *secondParam = strtok(NULL, paramsDelim);
    char *thirdParam = strtok(NULL, paramsDelim);
    
    // printf("Printing event:\n");
    // printf("Event:'%s'\n", event);
    if(strcmp(event, "NEW_USER") == 0){
        sInfo.numUsers += 1;
        // printf("sInfo.numUsers: %d\n", sInfo.numUsers);
    }else if(strcmp(event, "USER_TESTING_QUEUE") == 0){
        sInfo.numUsersInTestingQueue += 1;
        // printf("sInfo.numUsersInTestingQueue: %d\n", sInfo.numUsersInTestingQueue);
    }else if(strcmp(event, "USER_TEST_START") == 0){
        sInfo.totalUsersPassedInQueue += 1;
        sInfo.numUsersInTestingQueue -= 1;
        sInfo.numUsersDoingTheTest += 1;
        if(strcmp(secondParam, "0") == 0){
            sInfo.numUsersNormalTest += 1;
        }else{
            sInfo.numUsersFastTest += 1;
        }
        sInfo.totalSecondsPassedInQueue += atoi(thirdParam);
        // printf("sInfo.numUsersInTestingQueue: %d\n", sInfo.numUsersInTestingQueue);
        // printf("sInfo.numUsersDoingTheTest: %d\n", sInfo.numUsersDoingTheTest);
        // printf("sInfo.numUsersNormalTest: %d\n", sInfo.numUsersNormalTest);
        // printf("sInfo.numUsersFastTest: %d\n", sInfo.numUsersFastTest);
    }else if(strcmp(event, "USER_TEST_END") == 0){
        sInfo.numUsersDoneTheTest += 1;
        sInfo.numUsersDoingTheTest -= 1;
        sInfo.totalUsersDoneTests += 1;
        sInfo.totalSecondsDoingTests += atoi(secondParam);
        // printf("sInfo.numUsersDoneTheTest: %d\n", sInfo.numUsersDoneTheTest);
    }else if(strcmp(event, "USER_ISOLATION") == 0){
        sInfo.numUsersIsolated += 1;
        sInfo.totalUsersIsolated += 1;
        // printf("sInfo.numUsersIsolated: %d\n", sInfo.numUsersIsolated);
    }else if(strcmp(event, "NEGATIVE_TEST") == 0){
        sInfo.numNegativeTests += 1;
        sInfo.totalUsersWaitedTestResult += 1;
        sInfo.totalSecondsUntilTestResult += atoi(secondParam);
        sInfo.totalSecondsIsolated += atoi(thirdParam);
        // printf("sInfo.numNegativeTests: %d\n", sInfo.numNegativeTests);
    }else if(strcmp(event, "POSITIVE_TEST") == 0){
        sInfo.numPositiveTests += 1;
        sInfo.totalUsersWaitedTestResult += 1;
        sInfo.totalSecondsUntilTestResult += atoi(secondParam);
        // printf("sInfo.numPositiveTests: %d\n", sInfo.numPositiveTests);
    }else if(strcmp(event, "HOME_ISOLATION") == 0){
        sInfo.numUsersIsolatedInHome += 1;
        // printf("sInfo.numUsersIsolatedInHome: %d\n", sInfo.numUsersIsolatedInHome);
    }else if(strcmp(event, "DIED") == 0){
        sInfo.numDeaths += 1;
        sInfo.totalSecondsIsolated += atoi(secondParam);
        // printf("sInfo.numDeaths: %d\n", sInfo.numDeaths);
    }else if(strcmp(event, "HEALED") == 0){
        sInfo.numHealed += 1;
        sInfo.totalSecondsIsolated += atoi(secondParam);
        // printf("sInfo.numHealed: %d\n", sInfo.numHealed);
    }else if(strcmp(event, "END") == 0){
        sInfo.isRunning = 0;
        // printf("sInfo.isRunning: %d\n", sInfo.isRunning);
        // printMenu();
    }else if(strcmp(event, "HOSPITALIZATION") == 0){
        sInfo.numHospitalized += 1;
    }
    else if(strcmp(event, "UNHOSPITALIZATION") == 0){
        sInfo.numHospitalized -= 1;
    }
}

void monitor(void* arg) 
{ 
    pthread_detach(pthread_self());

    struct monitorThreadInfo *tInfo = (struct monitorThreadInfo*)arg;

    char buff[MAX_SOCKET_LENGTH]; 
    int n;
    struct timespec ts;
    ts.tv_nsec = 10000000;
    ts.tv_sec = 0;

    // infinite loop for chat 
    for (;;) { 
        bzero(buff, MAX_SOCKET_LENGTH); 
  
        // read the message from client and copy it in buffer 
        read((*tInfo).sockfd, buff, sizeof(buff));
        
        if(strlen(buff) > 0){
            // print buffer which contains the client contents 
            // printf("From client: %s\n", buff);
            appendToLogFile(buff);
            parseResponse(buff);
        }

        nanosleep(&ts, NULL);
    }

    printf("Closing the socket...\n");
    close((*tInfo).sockfd);
    printf("Closed the socket...\n");
}

void printMenu(){
    printf("Simulação em curso: %s\n", sInfo.isRunning ? "Sim" : "Não");
    printf("1) Número de pessoas criadas na simulação\n");
    printf("2) Número de pessoas na fila de testagem\n");
    printf("3) Número de pessoas a fazer o teste\n");
    printf("4) Número de pessoas que já fizeram o teste\n");
    printf("5) Número de pessoas que estão em isolamento\n");
    printf("6) Número de pessoas que estão em isolamento em casa\n");
    printf("7) Número de testes negativos\n");
    printf("8) Número de testes positivos\n");
    printf("9) Número de pessoas que faleceram\n");
    printf("10) Número de pessoas que recuperaram\n");
    printf("11) Número de hospitalizadas\n");
    printf("12) Tempo médio na fila de espera\n");
    printf("13) Tempo médio a fazer o teste\n");
    printf("14) Tempo médio para receber o resultado\n");
    printf("15) Tempo médio de isolamento\n");
    printf("16) Sair...\n\n\n");
}

int requestOption(){
    int option;
    printf("Introduza um opção: ");
    scanf("%d", &option);
    return option;
}

void processMenu(int option){
    int mean = 0;
    printf("\n\n\n");
    switch (option)
    {
        case 1:
            printf("Até agora foram criadas %d pessoas.", sInfo.numUsers);
            break;
        case 2:
            printf("Encontram-se %d pessoas na fila de testagem.", sInfo.numUsersInTestingQueue);
            break;
        case 3:
            printf("Encontram-se %d pessoas a fazer o teste.", sInfo.numUsersDoingTheTest);
            break;
        case 4:
            printf("%d pessoas já fizeram o teste.", sInfo.numUsersDoneTheTest);
            break;
        case 5:
            printf("Encontram-se %d pessoas em isolamento.", sInfo.numUsersIsolated);
            break;
        case 6:
            printf("Encontram-se %d pessoas em isolamento em casa.", sInfo.numUsersIsolatedInHome);
            break;
        case 7:
            printf("%d pessoas testaram negativo.", sInfo.numNegativeTests);
            break;
        case 8:
            printf("%d pessoas testaram positivo.", sInfo.numPositiveTests);
            break;
        case 9:
            printf("%d pessoas faleceram.", sInfo.numDeaths);
            break;
        case 10:
            printf("%d pessoas recuperaram.", sInfo.numHealed);
            break;
        case 11:
            printf("%d pessoas hospitalizadas.", sInfo.numHospitalized);
            break;
        case 12:
            if(sInfo.totalUsersPassedInQueue){
                mean = (int)(sInfo.totalSecondsPassedInQueue / sInfo.totalUsersPassedInQueue);
            }
            printf("%d segundos é o tempo médio na fila de espera.", mean);
            break;
        case 13:
            if(sInfo.totalUsersDoneTests){
                mean = (int)(sInfo.totalSecondsDoingTests / sInfo.totalUsersDoneTests);
            }
            printf("%d segundos é o tempo médio a fazer o teste.", mean);
            break;
        case 14:
            if(sInfo.totalUsersWaitedTestResult){
                mean = (int)(sInfo.totalSecondsUntilTestResult / sInfo.totalUsersWaitedTestResult);
            }
            printf("%d segundos é o tempo médio para receber o teste.", mean);
            break;
        case 15:
            if(sInfo.totalUsersIsolated){
                mean = (int)(sInfo.totalSecondsIsolated / sInfo.totalUsersIsolated);
            }
            printf("%d segundos é o tempo médio de isolamento.", mean);
            break;
        case 16:
            exit(0);
            break;
        
        default:
            printf("Por favor escolha o opção válida.\n");
            int option = requestOption();
            processMenu(option);
            break;
    }
    printf("\n\n\n");
}

void stats(){
    struct timespec ts;
    ts.tv_nsec = 10000000;
    ts.tv_sec = 0;

    for(;;){
        printMenu();
        int option = requestOption();
        processMenu(option);
    }
}

int main(int argc, char *argv[])
{
    if(argc != 2){
        printf("Por favor insira apenas um argumento ao inicializar o monitor");
        return 0;
    }

    emptyLogFile();

    struct Configuration config = getConfig(argv);

    printf("Getting the socket....\n");
    int sockfd = getSocket(config);
    printf("Got the socket: %d....\n", sockfd);

    pthread_t ptid;
    struct monitorThreadInfo *tInfo;
    tInfo = malloc(sizeof(struct monitorThreadInfo));
    (*tInfo).sockfd = sockfd;
    pthread_create(&ptid, NULL, &monitor, (void*)tInfo);

    stats();
}