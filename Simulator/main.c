// CLIENT
#include <pthread.h> 
#include <netdb.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <stdbool.h>
#include <sys/socket.h> 
#include <sys/types.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>
#include "configuration/parser.c"
#define SA struct sockaddr 
#define MAX_SOCKET_LENGTH 1024
#define TESTING_CENTERS_LENGTH 2
#define NURSES_BY_CENTER 1

sem_t testingCentersQueue[TESTING_CENTERS_LENGTH];
int currentNumberInHospital = 0;

pthread_mutex_t *hospitalizationMutex;
pthread_mutex_t *nursesPerCenter[TESTING_CENTERS_LENGTH][TESTING_CENTERS_LENGTH * NURSES_BY_CENTER];
pthread_mutex_t *socketMutex;

struct threadInfo {
    int id;
    int age;

    bool isFastTest;
    bool isHospitalized;

    long arrivedTestingCenterAt;

    long arrivedTestingQueueAt;
    long leftTestingQueueAt;
    
    long startTheTestAt;
    long endTheTestAt;
    long receivedResultAt;
    
    long beginIsolatedAt;
    long stopIsolatedAt;

    int testingCenterMutexIndex;
    int sockfd;

    struct Configuration config;
};

long getTimestamp(){
    return (long)time(NULL);
}

int getRandomNumber(int min, int max){
    return (rand() % (max - min + 1)) + min;
}

void sendMonitor(struct threadInfo tInfo,  char buff[MAX_SOCKET_LENGTH]){
    pthread_mutex_lock(&socketMutex);

    printf("Send: %s\n", buff);
    write(tInfo.sockfd, buff, MAX_SOCKET_LENGTH); 
    bzero(buff, MAX_SOCKET_LENGTH);

    pthread_mutex_unlock(&socketMutex);
}

int getSocket(struct Configuration config){
    int sockfd, connfd; 
    struct sockaddr_in servaddr, cli; 
  
    // socket create and varification 
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
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
    servaddr.sin_port = htons(config.connectPort); 
  
    // connect the client socket to server socket
    connfd = connect(sockfd, (SA*)&servaddr, sizeof(servaddr));
    if (connfd != 0) { 
        printf("connection with the server failed... Response: %d\n", connfd); 
        exit(0); 
    } 
    else{
        printf("connected to the server on the port %d, Response: %d...\n", config.connectPort, connfd); 
    }

    return sockfd;
}

void decreaseCurrentHospitalized(struct threadInfo *tInfo){
    pthread_mutex_lock(&hospitalizationMutex);
    
    (*tInfo).isHospitalized = false;
    
    char buffer [MAX_SOCKET_LENGTH];
    snprintf (buffer, MAX_SOCKET_LENGTH, "%s-%d", "UNHOSPITALIZATION", (*tInfo).id);
    sendMonitor(*tInfo, buffer);
    
    currentNumberInHospital -= 1;
    
    pthread_mutex_unlock(&hospitalizationMutex);
}

void tryToHospitalize(struct threadInfo *tInfo){
    if((*tInfo).isHospitalized){
        return;
    }

    pthread_mutex_lock(&hospitalizationMutex);

    printf("Current usage of the hospital: %d\n", currentNumberInHospital);

    if(currentNumberInHospital < (*tInfo).config.maxSlotsHospital){
        currentNumberInHospital += 1;
        (*tInfo).isHospitalized = true;

        char buffer[MAX_SOCKET_LENGTH];
        snprintf (buffer, MAX_SOCKET_LENGTH, "%s-%d", "HOSPITALIZATION", (*tInfo).id);
        sendMonitor(*tInfo, buffer);
    }else{
        printf("Can't be hospitalized because hospital is full....\n");
    }

    pthread_mutex_unlock(&hospitalizationMutex);
}

void newUserEvent(struct threadInfo *tInfo){
    char buffer [MAX_SOCKET_LENGTH];

    (*tInfo).arrivedTestingCenterAt = getTimestamp();
    // printf("tInfo.arrivedTestingCenterAt = %ld\n\n", (*tInfo).arrivedTestingCenterAt);
    snprintf ( buffer, MAX_SOCKET_LENGTH, "%s-%d,%d", "NEW_USER", (*tInfo).id, (*tInfo).age);
    sendMonitor(*tInfo, buffer);
    printf("The user %d arrived at the testing center!\n", (*tInfo).id);
}

void userArriveTestingQueue(struct threadInfo *tInfo){
    char buffer [MAX_SOCKET_LENGTH];

    sem_wait(&testingCentersQueue[(*tInfo).testingCenterMutexIndex]);

    (*tInfo).arrivedTestingQueueAt = getTimestamp();
    // printf("tInfo.arrivedTestingQueueAt = %ld \n\n", (*tInfo).arrivedTestingQueueAt);
    snprintf (buffer, MAX_SOCKET_LENGTH, "%s-%d", "USER_TESTING_QUEUE", (*tInfo).id);
    sendMonitor(*tInfo, buffer);
    printf("The user %d is inside the testing center and waiting to be tested!\n", (*tInfo).id);
}

void onUserStartTest(struct threadInfo *tInfo){
    char buffer [MAX_SOCKET_LENGTH];

    (*tInfo).startTheTestAt = getTimestamp();

    int diff = (int)((*tInfo).leftTestingQueueAt - (*tInfo).arrivedTestingQueueAt);
    // printf("tInfo.leftTestingQueueAt: %ld\n", (*tInfo).leftTestingQueueAt);
    // printf("tInfo.arrivedTestingQueueAt: %ld\n", (*tInfo).arrivedTestingQueueAt);
    // printf("diff: %d\n", diff);
    snprintf ( buffer, MAX_SOCKET_LENGTH, "%s-%d,%d,%ld", "USER_TEST_START", (*tInfo).id, (*tInfo).isFastTest, diff);
    sendMonitor(*tInfo, buffer);
    printf("The user %d is being tested and has been %d seconds in the queue\n", (*tInfo).id, diff);

    sleep((*tInfo).isFastTest ? (*tInfo).config.fastTestDurationSeconds : (*tInfo).config.normalTestDurationSeconds);
}

void onUserEndTest(struct threadInfo *tInfo){
    char buffer [MAX_SOCKET_LENGTH];

    (*tInfo).endTheTestAt = getTimestamp();

    int diff = (int) ((*tInfo).endTheTestAt - (*tInfo).startTheTestAt);
    // printf("tInfo.endTheTestAt: %ld\n", (*tInfo).endTheTestAt);
    // printf("tInfo.startTheTestAt: %ld\n", (*tInfo).startTheTestAt);
    // printf("diff: %d\n", diff);

    snprintf ( buffer, MAX_SOCKET_LENGTH, "%s-%d,%ld", "USER_TEST_END", (*tInfo).id, diff);
    sendMonitor(*tInfo, buffer);

    printf("The user %d finished to be tested, now he will wait until the result. The test took %d seconds\n", (*tInfo).id, diff);
}

void userDoTest(struct threadInfo *tInfo){
    char buffer [MAX_SOCKET_LENGTH];

    int randomNurse = getRandomNumber(1, NURSES_BY_CENTER);

    pthread_mutex_lock(&nursesPerCenter[(*tInfo).testingCenterMutexIndex][randomNurse]);
    
    (*tInfo).isFastTest = getRandomNumber(0, 1) <= (*tInfo).config.fastTestFactor;

    (*tInfo).leftTestingQueueAt = getTimestamp();

    onUserStartTest(tInfo);

    onUserEndTest(tInfo);

    pthread_mutex_unlock(&nursesPerCenter[(*tInfo).testingCenterMutexIndex][randomNurse]);

    sem_post(&testingCentersQueue[(*tInfo).testingCenterMutexIndex]);
}

void userIsolated(struct threadInfo *tInfo){
    char buffer [MAX_SOCKET_LENGTH];

    (*tInfo).beginIsolatedAt = getTimestamp();
    snprintf ( buffer, MAX_SOCKET_LENGTH, "%s-%d", "USER_ISOLATION", (*tInfo).id);
    sendMonitor(*tInfo, buffer);
    printf("The user %d will wait for the result of the %s test.\n", (*tInfo).id, (*tInfo).isFastTest ? "Fast" : "Normal");
    
    sleep((*tInfo).isFastTest ? (*tInfo).config.fastTestAnalysesSeconds : (*tInfo).config.normalTestAnalysesSeconds);
}

void onNegativeTest(struct threadInfo *tInfo){
    char buffer [MAX_SOCKET_LENGTH];

    (*tInfo).stopIsolatedAt = getTimestamp();

    int diffWaiting = (int)((*tInfo).receivedResultAt - (*tInfo).endTheTestAt);
    // printf("tInfo.receivedResultAt: %ld\n", (*tInfo).receivedResultAt);
    // printf("tInfo.endTheTestAt: %ld\n", (*tInfo).endTheTestAt);
    // printf("diffWaiting: %d\n", diffWaiting);
    int diffIsolated = (int)((*tInfo).stopIsolatedAt - (*tInfo).beginIsolatedAt);
    // printf("tInfo.stopIsolatedAt: %ld\n", (*tInfo).stopIsolatedAt);
    // printf("tInfo.beginIsolatedAt: %ld\n", (*tInfo).beginIsolatedAt);
    // printf("diffIsolated: %d\n", diffIsolated);
    
    snprintf ( buffer, MAX_SOCKET_LENGTH, "%s-%d,%ld,%ld", "NEGATIVE_TEST", (*tInfo).id, diffWaiting, diffIsolated);
    sendMonitor(*tInfo, buffer);
    
    printf("The user %d is ok so he can go...Waited: %d and Isolated: %d\n", (*tInfo).id, diffWaiting, diffIsolated);
    pthread_exit(NULL);
}

void onPositiveTest(struct threadInfo *tInfo){
    char buffer [MAX_SOCKET_LENGTH];

    int diff = (int) ((*tInfo).receivedResultAt - (*tInfo).endTheTestAt);
    // printf("tInfo.receivedResultAt: %ld\n", (*tInfo).receivedResultAt);
    // printf("tInfo.endTheTestAt: %ld\n", (*tInfo).endTheTestAt);
    // printf("diff: %d\n", diff);
    
    snprintf ( buffer, MAX_SOCKET_LENGTH, "%s-%d,%ld", "POSITIVE_TEST", (*tInfo).id, diff);
    sendMonitor(*tInfo, buffer);
    
    printf("The user %d is sick... waited %d for the result", (*tInfo).id, diff);
}

void onHomeIsolation(struct threadInfo *tInfo){
    char buffer [MAX_SOCKET_LENGTH];

    snprintf ( buffer, MAX_SOCKET_LENGTH, "%s-%d", "HOME_ISOLATION", (*tInfo).id);
    
    sendMonitor(*tInfo, buffer);
}

void onHealed(struct threadInfo *tInfo){
    char buffer [MAX_SOCKET_LENGTH];

    (*tInfo).stopIsolatedAt = getTimestamp();
    int diff = (int) ((*tInfo).stopIsolatedAt - (*tInfo).beginIsolatedAt);
    // printf("tInfo.stopIsolatedAt: %ld\n", (*tInfo).stopIsolatedAt);
    // printf("tInfo.beginIsolatedAt: %ld\n", (*tInfo).beginIsolatedAt);
    // printf("diff: %d\n", diff);

    snprintf ( buffer, MAX_SOCKET_LENGTH, "%s-%d,%ld", "HEALED", (*tInfo).id, diff);
    sendMonitor(*tInfo, buffer);
    printf("The user %d is healed! Tool %d seconds isolated\n", (*tInfo).id, diff);
}

void* flow(void* arg) 
{ 
    // detach the current thread 
    // from the calling thread
    char buffer [MAX_SOCKET_LENGTH];
    
    pthread_detach(pthread_self());

    struct threadInfo *tInfo = (struct threadInfo*)arg;

    newUserEvent(&(*tInfo));

    userArriveTestingQueue(&(*tInfo));

    userDoTest(&(*tInfo));

    userIsolated(&(*tInfo));
    
    (*tInfo).receivedResultAt = getTimestamp();

    bool isSick = getRandomNumber(0, 1) <= (*tInfo).config.infectedFactor;
    isSick = true;
    if(!isSick){
        onNegativeTest(&(*tInfo));
    }

    onPositiveTest(&(*tInfo));

    bool isOld = (*tInfo).age >= (*tInfo).config.hospitalizationAge;
    float factor = isOld ? (*tInfo).config.oldPeopleDieFactor : (*tInfo).config.youngPeopleDieFactor;
    printf("The user %d on initially had %.6f of probability to die!\n", (*tInfo).id, factor);
    int day = 1;
    int healedDay = getRandomNumber((*tInfo).config.minDaysSick, (*tInfo).config.maxDaysSick);

    if(isOld){
        printf("The user %d is %d old, so will be hospitalized...\n", (*tInfo).id, (*tInfo).age);
        tryToHospitalize(&(*tInfo));
        if(!(*tInfo).isHospitalized){
            factor += (*tInfo).config.perDayFactor;

            onHomeIsolation(&(*tInfo));
            
            printf("The user %d can't be hospitalized, so the factor now is: %f...\n", (*tInfo).id, factor);
        }
    }else{
        onHomeIsolation(&(*tInfo));
        
        printf("The user %d is %d old, so will be stay home...\n", (*tInfo).id, (*tInfo).age);
    }


    while(day <= healedDay){
        sleep(1);
        printf("Another day the user %d is sick! Current day: %dth\n", (*tInfo).id, day);

        int random = getRandomNumber(1, 100);

        bool hasDied = random <= (factor * 100);
        if(hasDied){
            if((*tInfo).isHospitalized){
                decreaseCurrentHospitalized(tInfo);
            }
            (*tInfo).stopIsolatedAt = getTimestamp();

            int diffIsolated = (int) ((*tInfo).stopIsolatedAt - (*tInfo).beginIsolatedAt);
            // printf("tInfo.stopIsolatedAt: %ld\n", (*tInfo).stopIsolatedAt);
            // printf("tInfo.beginIsolatedAt: %ld\n", (*tInfo).beginIsolatedAt);
            // printf("diffIsolated: %d\n", diffIsolated);

            snprintf ( buffer, MAX_SOCKET_LENGTH, "%s-%d,%ld", "DIED", (*tInfo).id, diffIsolated);
            sendMonitor(*tInfo, buffer);
            
            printf("The user %d is dead! Took %d isolated\n", (*tInfo).id, diffIsolated);
            pthread_exit(NULL);
        }

        if(isOld && !(*tInfo).isHospitalized){
            tryToHospitalize(&(*tInfo));
            if(!(*tInfo).isHospitalized){
                factor += (*tInfo).config.perDayFactor;
            }
        }
        printf("The user %d on the %dth had %f of probability to die!\n", (*tInfo).id, day, factor);
        day++;
    }

    if((*tInfo).isHospitalized){
        decreaseCurrentHospitalized(tInfo);
    }

    onHealed(&(*tInfo));
    
    pthread_exit(NULL);
}

void simulate(int sockfd, struct Configuration config) 
{
    pthread_t threads[config.numberOfThreads];

    struct threadInfo *tInfo;

    for (int i = 0; i < config.numberOfThreads; i++)
    {
        printf("Creating the %d thread\n", i +1);

        // printf("Allocating memory for the struct for the thread %d\n", i + 1);

        tInfo = malloc(sizeof(struct threadInfo));

        (*tInfo).sockfd = sockfd;
        (*tInfo).config = config;
        (*tInfo).testingCenterMutexIndex = i % TESTING_CENTERS_LENGTH;
        (*tInfo).id = i + 1;
        (*tInfo).age = getRandomNumber(config.minAge, config.maxAge);

        pthread_create(&threads[i], NULL, &flow, (void*)tInfo);
    }

    for (int i = 0; i < config.numberOfThreads; i++)
    {
        pthread_join(threads[i], NULL);
    }
}

void sendEnd(int sockfd){
    char buffer [MAX_SOCKET_LENGTH];
    snprintf (buffer, MAX_SOCKET_LENGTH, "%s-%d", "END",1);
    write(sockfd, buffer, MAX_SOCKET_LENGTH);
}

int main(int argc, char *argv[])
{
    srand(time(NULL));

    if(argc != 2){
        printf("Por favor insira apenas um argumento ao inicializar o monitor");
        return 0;
    }

    struct Configuration config = getConfig(argv);

    printf("connectPort: %d\n", config.connectPort);
    printf("numberOfThreads: %d\n", config.numberOfThreads);
    printf("queueMaxSize: %d\n", config.queueMaxSize);
    printf("infectedFactor: %f\n", config.infectedFactor);
    printf("fastTestFactor: %f\n", config.fastTestFactor);
    printf("fastTestDurationSeconds: %d\n", config.fastTestDurationSeconds);
    printf("normalTestDurationSeconds: %d\n", config.normalTestDurationSeconds);
    printf("fastTestAnalysesSeconds: %d\n", config.fastTestAnalysesSeconds);
    printf("normalTestAnalysesSeconds: %d\n", config.normalTestAnalysesSeconds);
    printf("oldPeopleDieFactor: %f\n", config.oldPeopleDieFactor);
    printf("youngPeopleDieFactor: %f\n", config.youngPeopleDieFactor);
    printf("perDayFactor: %f\n", config.perDayFactor);
    printf("minAge: %d\n", config.minAge);
    printf("maxAge: %d\n", config.maxAge);
    printf("hospitalizationAge: %d\n", config.hospitalizationAge);
    printf("minDaysSick: %d\n", config.minDaysSick);
    printf("maxDaysSick: %d\n", config.maxDaysSick);
    printf("maxSlotsHospital: %d\n", config.maxSlotsHospital);
    printf("numberOfTestingCenters: %d\n", config.numberOfTestingCenters);
    printf("numberOfNursesPerCenter: %d\n", config.numberOfNursesPerCenter);

    sleep(5);

    for (int i = 0; i < TESTING_CENTERS_LENGTH; i++)
    {   
        for (int j = 0; j < NURSES_BY_CENTER; j++)
        {
            if (pthread_mutex_init(&nursesPerCenter[i][j], NULL) != 0)
            {
                printf("Mutex init failed\n");
                return 1;
            }
        }
    }

    if (pthread_mutex_init(&socketMutex, NULL) != 0)
    {
        printf("Mutex init failed\n");
        return 1;
    }

    if (pthread_mutex_init(&hospitalizationMutex, NULL) != 0)
    {
        printf("Mutex init failed\n");
        return 1;
    }

    for (int w = 0; w < TESTING_CENTERS_LENGTH; w++)
    {
        sem_init(&testingCentersQueue[0], 0, 2);
        // printSemValue(testingCentersQueue[0]);
    }

    currentNumberInHospital = config.maxSlotsHospital;

    printf("Testing centers initialized!\n");

    printf("Getting the socket...\n");
    int sockfd = getSocket(config);
    printf("Got the socket: %d...\n", sockfd);

    printf("Starting the simulation...\n");
    simulate(sockfd, config);
    printf("End of the simulation...\n");

    sendEnd(sockfd);

    clean(sockfd);
}

void printSemValue(sem_t semaphore){
    int sem_value = 9999;
    sem_getvalue(&semaphore, &sem_value);
    printf("Value of semaphore -> %d\n", sem_value);
}

void clean(int sockfd){
    printf("Closing the socket...\n");
    close(sockfd);
    printf("Closed the socket...\n");

    for (int i = 0; i < TESTING_CENTERS_LENGTH; i++)
    {
        sem_destroy(&(testingCentersQueue[i]));
        for (int j = 0; j < NURSES_BY_CENTER; j++)
        {
            pthread_mutex_destroy(&nursesPerCenter[i][j]);
        }
    }

    pthread_mutex_destroy(&socketMutex);
    pthread_mutex_destroy(&hospitalizationMutex);
}