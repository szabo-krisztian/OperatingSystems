#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include "menuoptions.h"
#include "utilityfunctions.h"

const char* FILE_NAME = "versek_fix.txt";
const int NUMBER_OF_CHILDREN = 4;
const int PIPE_READ_WRITE = 2;

void read_poem(FILE* file, char* title) {
    rewind(file);

    char line[MAX_STR_LENGTH];
    while (fgets(line, sizeof(line), file) && strcmp(line, title) != 0) {}
    fgets(line, sizeof(line), file);
    printf("Here goes your poem:\n%s", line);
}

void write_poem(FILE* file, char* title) {
    char poem[MAX_STR_LENGTH];
    printf("Write down your poem: \n");
    fgets(poem, sizeof(poem), stdin);
    fprintf(file, title);
    fprintf(file, poem);
}

void edit_poem(FILE** file, char* title)
{
    read_poem(*file, title);
    printf("\nEnter the edited version: \n");

    char edited_poem[MAX_STR_LENGTH];
    fgets(edited_poem, MAX_STR_LENGTH, stdin);
    
    const char* TEMP_FILE_NAME = "temp.txt";
    FILE* temp_file  = fopen(TEMP_FILE_NAME, "w");
    if (temp_file == NULL) {
        perror("Error opening temporary file!");
        exit(1);
    }

    rewind(*file);
    char line[MAX_STR_LENGTH];
    while (fgets(line, sizeof(line), *file)) {
        if (strcmp(line, title) == 0) {
            skip_lines_in_file(*file, 1);
            fputs(title, temp_file);
            fputs(edited_poem, temp_file);
        } else {
            fputs(line, temp_file);
        }
    }
    
    fclose(*file);
    fclose(temp_file);

    remove(FILE_NAME);
    rename(TEMP_FILE_NAME, FILE_NAME);
    
    *file = fopen(FILE_NAME, "r+");
}

void delete_poem(FILE** file, char* title) {
    rewind(*file);
    const char* TEMP_FILE_NAME = "temp.txt";
    FILE* temp_file  = fopen(TEMP_FILE_NAME, "w");
    if (temp_file == NULL) {
        perror("Error opening temporary file!");
        exit(1);
    }

    char line[MAX_STR_LENGTH];
    int line_index = 0;
    while (fgets(line, sizeof(line), *file)) {
        if (strcmp(line, title)) {
            skip_lines_in_file(*file, 1);
        } else {
            fputs(line, temp_file);
        }
    }

    fclose(*file);
    fclose(temp_file);

    remove(FILE_NAME);
    rename(TEMP_FILE_NAME, FILE_NAME);
    
    *file = fopen(FILE_NAME, "r+");
}

void child_started_journey_handler(int signumber)
{
    printf("Child named %d reached his desination.\n", getpid());
}

void child_reached_destination_handler(int signumber)
{
    printf("Parent starts sending poems\n");
}

void start_child_listening(int* pipefd, FILE* file, int uzenetsor)
{
    while (1)
    {
        pause();
        sleep(1);
        kill(getppid(), SIGUSR2);

        int message_length;
        TitlePair received_pair;

        read(pipefd[0], &message_length, sizeof(int));
        read(pipefd[0], &received_pair, message_length);
        
        char poem1[MAX_STR_LENGTH];
        char poem2[MAX_STR_LENGTH];
        get_poems_from_indexes(poem1, poem2, received_pair.title1_index, received_pair.title2_index, file);
        printf("Here goes the first poem:\n%s", poem1);
        printf("Here goes the second poem:\n%s", poem2);

        kuld(uzenetsor);
    }
}

void init_pipes(int pipes[NUMBER_OF_CHILDREN][PIPE_READ_WRITE])
{
    for (int i = 0; i < NUMBER_OF_CHILDREN; ++i)
    {
        if (pipe(pipes[i]) == -1)
        {
            perror("Pipe opening error\n");
            exit(EXIT_FAILURE);
        }
    }
}

void init_children(int children[NUMBER_OF_CHILDREN], int pipes[NUMBER_OF_CHILDREN][PIPE_READ_WRITE], FILE* file, int uzenetsor)
{
    for (int i = 0; i < NUMBER_OF_CHILDREN; ++i)
    {
        pid_t id = fork();
        if (id == -1)
        {
            perror("Fork error\n");
            exit(EXIT_FAILURE);
        }

        if (id == 0)
        {
            start_child_listening(pipes[i], file, uzenetsor);
            exit(EXIT_SUCCESS);
        }
        children[i] = id;
    }
}

void tell_children_to_travel(FILE* file, pid_t children[NUMBER_OF_CHILDREN], int pipes[NUMBER_OF_CHILDREN][PIPE_READ_WRITE], int uzenetsor)
{
    int number_of_poems = get_number_of_poems(file);
    if (number_of_poems < 2)
    {
        return;
    }

    int random_id = get_random_number(NUMBER_OF_CHILDREN);
    sleep(1);
    kill(children[random_id], SIGTERM);
    pause();

    int title_indexes[2];
    get_two_random_title_index(title_indexes, file);

    TitlePair pair = {title_indexes[0], title_indexes[1]};
    int message_length = sizeof(pair);
    
    write(pipes[random_id][1], &message_length, sizeof(int));
    write(pipes[random_id][1], &pair, message_length);

    fogad(uzenetsor);
}

    
void kill_children(pid_t children[NUMBER_OF_CHILDREN])
{
    sleep(1);
    for (int i = 0; i < NUMBER_OF_CHILDREN; ++i)
    {
        kill(children[i], SIGUSR1);
    }

    int status;
    for (int i = 0; i < 4; ++i)
    {
        waitpid(children[i], &status, 0);
    }
}

int main(int argc, char* argv[])
{
    FILE* file;
    file = fopen(FILE_NAME, "r+");

    int uzenetsor, status; 
    key_t kulcs; 
    kulcs = ftok(argv[0], 1); 
    uzenetsor = msgget(kulcs, 0600 | IPC_CREAT); 

    signal(SIGTERM, child_started_journey_handler);
    signal(SIGUSR2, child_reached_destination_handler);

    int pipes[NUMBER_OF_CHILDREN][PIPE_READ_WRITE];
    pid_t children[NUMBER_OF_CHILDREN];
    init_pipes(pipes);
    init_children(children, pipes, file, uzenetsor);

    tell_children_to_travel(file, children, pipes, uzenetsor);
    kill_children(children);

    fclose(file);
    return 0;
}