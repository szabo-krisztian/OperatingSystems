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
    while (fgets(line, sizeof(line), file) && strcmp(line, title) != 0)
    {
        
    }
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
        if (strcmp(line, title) == 0) {
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

int random_id;

void child_started_journey_handler(int signumber)
{
    
}

void child_reached_destination_handler(int signumber)
{

}

int get_random_from_two_titles(int title1, int title2)
{
    int random_num = get_random_number(2);
    if (random_num == 0)
    {
        return title1;
    }
    return title2;
}

int send_message(int message_q, int title_index, FILE* file) 
{ 
    TitleFilePair pair = { title_index, file };
    const Message message = { 5, pair }; 

    msgsnd( message_q, &message, sizeof(TitleFilePair) , 0 ); 
} 
     
void receive_message( int message_q, FILE** file) 
{ 
    Message message; 
    msgrcv(message_q, &message, sizeof(TitleFilePair), 5, 0 );
    
    char title[MAX_STR_LENGTH];
    get_title_from_index(title, message.pair.title_index, *file);

    printf("Parent received the selected poem, called %s\n", title);
    //delete_poem(file, title);
}

void start_child_listening(int* pipefd, FILE* file, int message_q)
{
    while (1)
    {
        pause();
        printf("The %d. child reached his desination.\n", random_id + 1);

        sleep(1);
        kill(getppid(), SIGUSR2);

        int message_length;
        TitlePair received_pair;

        read(pipefd[0], &message_length, sizeof(int));
        read(pipefd[0], &received_pair, message_length);
        
        char title1[MAX_STR_LENGTH];
        char title2[MAX_STR_LENGTH];
        
        get_title_from_index(title1, received_pair.title1_index, file);
        get_title_from_index(title2, received_pair.title2_index, file);

        char poem1[MAX_STR_LENGTH];
        char poem2[MAX_STR_LENGTH];
        
        get_poem_from_title(poem1, title1, file);
        get_poem_from_title(poem2, title2, file);
        
        printf("\nThe first one is called: %s\t", title1);
        printf("%s", poem1);
        printf("The second one is called: %s\t", title2);
        printf("%s\n", poem2);

        int random_title = get_random_from_two_titles(received_pair.title1_index, received_pair.title2_index);
        char title_selected[MAX_STR_LENGTH];
        get_title_from_index(title_selected, random_title, file);
        printf("I selected the poem, called %s", title_selected);

        char poem_selected[MAX_STR_LENGTH];
        get_poem_from_index(poem_selected, random_title, file);        
        
        send_message(message_q, random_title, file);
        pause();
        printf("%sIs it okay to sprinkle?\n", poem_selected);
        sleep(1);
        kill(getppid(), SIGUSR2);
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

void init_children(int children[NUMBER_OF_CHILDREN], int pipes[NUMBER_OF_CHILDREN][PIPE_READ_WRITE], FILE* file, int message_q)
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
            start_child_listening(pipes[i], file, message_q);
            exit(EXIT_SUCCESS);
        }
        children[i] = id;
    }
}

void tell_children_to_travel(FILE** file, pid_t children[NUMBER_OF_CHILDREN], int pipes[NUMBER_OF_CHILDREN][PIPE_READ_WRITE], int message_q)
{
    int number_of_poems = get_number_of_poems(*file);
    if (number_of_poems < 2)
    {
        printf("Not enough poems.\n");
        return;
    }

    get_random_number(NUMBER_OF_CHILDREN);
    sleep(3);
    kill(children[random_id], SIGTERM);
    pause();
    printf("Parent starts sending poems.\n");

    int title_indexes[2];
    get_two_random_title_index(title_indexes, *file);

    TitlePair pair = {title_indexes[0], title_indexes[1]};
    int message_length = sizeof(pair);
    
    write(pipes[random_id][1], &message_length, sizeof(int));
    write(pipes[random_id][1], &pair, message_length);

    receive_message(message_q, file);
    sleep(1);
    kill(children[random_id], SIGTERM);
    pause();
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

void get_title_from_user(char* title)
{
    printf("Enter the title of the poem: ");
    fgets(title, MAX_STR_LENGTH, stdin);
}

void process_input(FILE** file, int answer, pid_t children[NUMBER_OF_CHILDREN], int pipes[NUMBER_OF_CHILDREN][PIPE_READ_WRITE], int message_q)
{
    char title[MAX_STR_LENGTH];

    switch (answer) {
        case READ:
            get_title_from_user(title);
            read_poem(*file, title);
            break;
        case WRITE:
            get_title_from_user(title);
            write_poem(*file, title);
            break;
        case EDIT:
            get_title_from_user(title);
            edit_poem(file, title);
            break;
        case SEND_CHILD:
            tell_children_to_travel(file, children, pipes, message_q);
            break;
        case DELETE:
            get_title_from_user(title);
            delete_poem(file, title);
            break;
    }
}

void read_answer(int* answer) {
    scanf("%d", answer);
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int main(int argc, char* argv[])
{
    FILE* file;
    file = fopen(FILE_NAME, "r+");

    int message_q, status; 
    key_t kulcs; 
    kulcs = ftok(argv[0], 1); 
    message_q = msgget(kulcs, 0600 | IPC_CREAT); 

    signal(SIGTERM, child_started_journey_handler);
    signal(SIGUSR2, child_reached_destination_handler);

    int pipes[NUMBER_OF_CHILDREN][PIPE_READ_WRITE];
    pid_t children[NUMBER_OF_CHILDREN];
    init_pipes(pipes);
    init_children(children, pipes, file, message_q);

    int answer;
    while (answer != QUIT) {
        print_menu(file);
        read_answer(&answer);
        process_input(&file, answer, children, pipes, message_q);
    }
    
    kill_children(children);

    fclose(file);
    return 0;
}