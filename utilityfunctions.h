#ifndef UTILITY_FUNCTIONS_H
#define UTILITY_FUNCTIONS_H

#include <time.h>
#include <sys/ipc.h> 
#include <sys/msg.h> 
#include <sys/types.h>

const int MAX_STR_LENGTH = 512;

typedef struct {
    int title1_index;
    int title2_index;
} TitlePair;

typedef struct { 
    long mtype;
    char mtext[1024]; 
} Message;

int kuld( int uzenetsor ) 
{ 
    const Message uz = { 5, "Hajra Fradi!" }; 
    msgsnd( uzenetsor, &uz, strlen ( uz.mtext ) + 1 , 0 ); 
} 
     
void fogad( int uzenetsor ) 
{ 
    Message uz; 
    msgrcv(uzenetsor, &uz, 1024, 5, 0 ); 
    printf( "A kapott uzenet kodja: %ld, szovege:  %s\n", uz.mtype, uz.mtext ); 
} 

int get_number_of_poems(FILE* file)
{
    rewind(file);

    char line[MAX_STR_LENGTH];
    int line_index = 0;
    
    while (fgets(line, sizeof(line), file))
    {
        ++line_index;
    }
    
    return line_index / 2;
}

int get_random_number(int upper_limit)
{
    srand(time(NULL));
    return rand() % upper_limit;
}

void get_title_from_index(char* title, int title_index, FILE* file)
{
    rewind(file);

    char line[MAX_STR_LENGTH];
    int line_index = 0;
    while (fgets(line, sizeof(line), file))
    {
        if (line_index / 2 == title_index)
        {
            strcpy(title, line);
            return;
        }
        ++line_index;
    }
}

void get_poem_from_title(char* poem, char* title, FILE* file)
{
    rewind(file);
    char line[MAX_STR_LENGTH];
    while (fgets(line, MAX_STR_LENGTH, file) && strcmp(title, line) != 0) { }
    fgets(poem, MAX_STR_LENGTH, file);
}

void skip_lines_in_file(FILE* file, int amount) {
    char line[MAX_STR_LENGTH];
    for (int i = 0; i < amount; ++i) {
        fgets(line, sizeof(line), file);
    }
}

void get_two_random_title_index(int* random_titles, FILE* file)
{
    int max_poems = get_number_of_poems(file);
    random_titles[0] = get_random_number(max_poems);
    random_titles[1] = get_random_number(max_poems);
    while (random_titles[1] == random_titles[0])
    {
        random_titles[1] = get_random_number(max_poems);
    }
}

void get_two_random_poems(char* poem1, char* poem2, FILE* file)
{
    int random_titles[2];
    get_two_random_title_index(random_titles, file);
    char title1[MAX_STR_LENGTH];
    get_title_from_index(title1, random_titles[0], file);
    char title2[MAX_STR_LENGTH];
    get_title_from_index(title2, random_titles[1], file);

    get_poem_from_title(poem1, title1, file);
    get_poem_from_title(poem2, title2, file);
}

void print_poem_titles(FILE* file)
{
    rewind(file);

    char line[MAX_STR_LENGTH];
    int line_index = 0;
    while (fgets(line, MAX_STR_LENGTH, file))
    {
        if (line_index % 2 == 0)
        {
            printf("    %s", line);
        }
        ++line_index;
    }
}

void get_poems_from_indexes(char* poem1, char* poem2, int title1_index, int title2_index, FILE* file)
{
    char title1[MAX_STR_LENGTH];
    char title2[MAX_STR_LENGTH];
    get_title_from_index(title1, title1_index, file);
    get_title_from_index(title2, title2_index, file);
    get_poem_from_title(poem1, title1, file);
    get_poem_from_title(poem2, title2, file);
}

#endif /* UTILITY_FUNCTIONS_H */