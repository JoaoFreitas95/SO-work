#include <stdbool.h>
#include <string.h> 

#define MAX_LINE_LENGTH 256
#define LISTEN_PORT_KEY "listenPort"

struct Configuration
{
    int listenPort;
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
        
        if(startsWith(key, LISTEN_PORT_KEY)){
            config.listenPort = atoi(value);
        }
    }
    
    /* Close file */
    if (fclose(file))
    {
        perror(argv[1]);
    }

    return config;
}