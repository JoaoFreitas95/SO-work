#include <stdbool.h>
#include <string.h> 

#define MAX_LINE_LENGTH 1024

#define CONNECT_PORT_KEY "connectPort"
#define NUMBER_OF_THREADS_KEY "numberOfThreads"
#define QUEUE_MAX_SIZE_KEY "queueMaxSize"
#define INFECTED_FACTOR "infectedFactor"
#define FAST_TEST_FACTOR_KEY "fastTestFactor"
#define FAST_TEST_DURATION_SECONDS_KEY "fastTestDurationSeconds"
#define NORMAL_TEST_DURATION_SECONDS_KEY "normalTestDurationSeconds"
#define FAST_TEST_ANALYSES_SECONDS_KEY "fastTestAnalysesSeconds"
#define NORMAL_TEST_ANALYSES_SECONDS_KEY "normalTestAnalysesSeconds"
#define OLD_PEOPLE_DIE_FACTOR_KEY "oldPeopleDieFactor"
#define YOUNG_PEOPLE_DIE_FACTOR_KEY "youngPeopleDieFactor"
#define PER_DAY_FACTOR_KEY "perDayFactor"
#define MIN_DAYS_SICK_KEY "minDaysSick"
#define MAX_DAYS_SICK_KEY "maxDaysSick"
#define MIN_AGE_KEY "minAge"
#define MAX_AGE_KEY "maxAge"
#define HOSPITALIZATION_AGE_KEY "hospitalizationAge"
#define MAX_SLOTS_HOSPITAL_KEY "maxSlotsHospital"
#define NUMBER_OF_TESTING_CENTERS_KEY "numberOfTestingCenters"
#define NUMBER_OF_NURSES_PER_CENTER_KEY "numberOfNursesPerCenter"

struct Configuration
{
    int connectPort;
    int numberOfThreads;
    int queueMaxSize;
    float infectedFactor;
    float fastTestFactor;
    int fastTestDurationSeconds;
    int normalTestDurationSeconds;
    int fastTestAnalysesSeconds;
    int normalTestAnalysesSeconds;
    float oldPeopleDieFactor;
    float youngPeopleDieFactor;
    float perDayFactor;
    int minAge;
    int maxAge;
    int hospitalizationAge;
    int minDaysSick;
    int maxDaysSick;
    int maxSlotsHospital;
    int numberOfTestingCenters;
    int numberOfNursesPerCenter;
};

bool startsWith(const char *array, const char *fragment) {
    for (int i = 0; fragment[i] != '\0'; i++) {
        if (array[i] != fragment[i]) {
            return false;
        }
    }

    return true;
}

int indexof(char *string, char *el){
    char* e;
    int index;

    e = strchr(string, el);
    index = (int)(e - string);
    return index;
}

void substring(char s[], char sub[], int p, int l) {
   int c = 0;
   
   while (c < l) {
      sub[c] = s[p+c];
      c++;
   }
   sub[c] = '\0';
}

struct Configuration getConfig(char *argv[]){
    char line[MAX_LINE_LENGTH];
    int line_count = 0;
    char delim[] = ":";

    FILE *file = fopen(argv[1], "r");
    struct Configuration config;
    
    if (!file)
    {
        perror(argv[1]);
    }
    
    /* Get each line until there are none left */
    while (fgets(line, MAX_LINE_LENGTH, file))
    {
        char key[MAX_LINE_LENGTH];
        char value[MAX_LINE_LENGTH];
        int index = indexof(line, ':');
        int lineSize = strlen(line);
        
        substring(line, key, 0, index);
        substring(line, value, index + 1, lineSize - index);
        
        if(startsWith(key, CONNECT_PORT_KEY)){
            config.connectPort = atoi(value);
        }else if(startsWith(key, NUMBER_OF_THREADS_KEY)){
            config.numberOfThreads = atoi(value);
        }else if(startsWith(key, QUEUE_MAX_SIZE_KEY)){
            config.queueMaxSize = atoi(value);
        }else if(startsWith(key, INFECTED_FACTOR)){
            config.infectedFactor = atof(value);
        }else if(startsWith(key, FAST_TEST_FACTOR_KEY)){
            config.fastTestFactor = atof(value);
        }else if(startsWith(key, FAST_TEST_DURATION_SECONDS_KEY)){
            config.fastTestDurationSeconds = atoi(value);
        }else if(startsWith(key, NORMAL_TEST_DURATION_SECONDS_KEY)){
            config.normalTestDurationSeconds = atoi(value);
        }else if(startsWith(key, FAST_TEST_ANALYSES_SECONDS_KEY)){
            config.fastTestAnalysesSeconds = atoi(value);
        }else if(startsWith(key, NORMAL_TEST_ANALYSES_SECONDS_KEY)){
            config.normalTestAnalysesSeconds = atoi(value);
        }else if(startsWith(key, OLD_PEOPLE_DIE_FACTOR_KEY)){
            config.oldPeopleDieFactor = atof(value);
        }else if(startsWith(key, YOUNG_PEOPLE_DIE_FACTOR_KEY)){
            config.youngPeopleDieFactor = atof(value);
        }else if(startsWith(key, PER_DAY_FACTOR_KEY)){
            config.perDayFactor = atof(value);
        }else if(startsWith(key, MIN_DAYS_SICK_KEY)){
            config.maxDaysSick = atoi(value);
        }else if(startsWith(key, MAX_DAYS_SICK_KEY)){
            config.minDaysSick = atoi(value);
        }else if(startsWith(key, MIN_AGE_KEY)){
            config.minAge = atoi(value);
        }else if(startsWith(key, MAX_AGE_KEY)){
            config.maxAge = atoi(value);
        }else if(startsWith(key, HOSPITALIZATION_AGE_KEY)){
            config.hospitalizationAge = atoi(value);
        }else if(startsWith(key, MAX_SLOTS_HOSPITAL_KEY)){
            config.maxSlotsHospital = atoi(value);
        }
        else if(startsWith(key, NUMBER_OF_TESTING_CENTERS_KEY)){
            config.numberOfTestingCenters = atoi(value);
        }else if(startsWith(key, NUMBER_OF_NURSES_PER_CENTER_KEY)){
            config.numberOfNursesPerCenter = atoi(value);
        }
    }
    
    /* Close file */
    if (fclose(file))
    {
        perror(argv[1]);
    }

    return config;
}