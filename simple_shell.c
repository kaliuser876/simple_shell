// TODO: Make a small shell
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>

#define EXIT_SUCCESS 0 // Setting a default value for successful exits.
#define EXIT_FAILURE 1 // Setting a value for failure exits.
#define LSH_RL_BUFSIZE 1024 // Defining character buffer size
#define LSH_TOK_BUFSIZE 64 // Define token buffer size
#define LSH_TOK_DELIM " \t\r\n\a"   // Some delemeters that will break up tokens

char *lsh_read_line(){
    // Reads in the next line
    int bufsize=  LSH_RL_BUFSIZE; // Pulling the value that is defined at the top
    int position = 0; // Starting point
    char *buffer = malloc(sizeof(char) * bufsize); // malloc sets aside memory dynamically
    int c; // This is going to keep track of the ascii value we are reading in

    if(!buffer){
        // Check to make sure the buffor was created, if it wasnt then there was an error
        fprintf(stderr, "lsh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    // There is allocated space for the buffer
    // I am doing the most of the same thing as getline() but wanted the experience of doing it this way. 
    while(1){
        // Read a character 
        c = getchar(); // Taking the first character

        // When we hit EOF, replace it with a null character and return the buffer, or hit a new line
        if(c == EOF || c == '\n'){ // EOF is a special character that shows we are at the end of the file, also EOF is a int not a character
            buffer[position] = '\0'; // \0 is the null character
            return buffer; // Return the buffer, which is a line of characters
        }
        else{
            // If we are not at EOF, then we want to add the character to the buffer at the current position.
            buffer[position] = c;
        }
        // After we added the character to the current postion, we have to update positon to the next index.
        position++;

        // We want to make sure that we gave enough room for the buffer, so we should check if we exceeded the buffer size.
        // If we did exceed the size, then we reallocate more space.
        if(position >= bufsize){
            bufsize += LSH_RL_BUFSIZE; // Add another chunk
            buffer = realloc(buffer, bufsize); // We tell the computer to give us more space
            if(!buffer){
                // Check to make sure the buffor was created, if it wasnt then there was an error
                fprintf(stderr, "lsh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

char **lsh_split_line(char *line){
    int bufsize = LSH_TOK_BUFSIZE; // Set the buffersize for tokens
    int position = 0; // Start at 0
    char **tokens = malloc(bufsize * sizeof(char*)); // Allocating space for our token buffer
    char *token; // Individual tokens

    // Check if tokens was allocated correctly
    if(!tokens){
        // Not allocated correctly, throw an error
        fprintf(stderr, "lsh: allocation error");
        exit(EXIT_FAILURE);
    }
    // It was allocated correctly, continue

    token = strtok(line, LSH_TOK_DELIM); // Check line and take up untill the next delemiter
    while(token != NULL){
        tokens[position] = token; // Set tokens at the current postion to token
        position++; // Move to the next spot

        // Make sure there is enough space in our buffer
        if(position >= bufsize){
            bufsize += LSH_TOK_BUFSIZE; // Add another chunk to our buffer size
            tokens = realloc(tokens, bufsize * sizeof(char*));  // Ask for more space and reasine to tokens
            if(!tokens){
                // Not allocated correctly, throw an error
                fprintf(stderr, "lsh: allocation error");
                exit(EXIT_FAILURE);
            }
        }
        // Grab the next token
        token = strtok(NULL, LSH_TOK_DELIM); // We pass it NULL to read from the same line
        // NOTE: if we would have wrote
        // token = strtok(line, LSH_TOK_DELIM)
        // Then we would have started at the beginning of the line, instead of continuing where we were at. 

        // Start the loop at the top
    }
    // Outside of the loop, means we have read all of the tokens 
    tokens[position] = NULL; // So we know where to end
    return tokens; // Sending the tokens back
}

int lsh_launch(char **args){
    pid_t pid, wpid; // process identifer, worker process identifier
    int status;

    pid = fork(); // This will create a copy of the running process, giving us 2 processes
    if(pid == 0){
        // Checking if this is the child process
        if( execvp(args[0], args) == -1){
            perror("lsh");
        }
        exit(EXIT_FAILURE);
    }
    else{
        // Parent process
        do {
            wpid = waitpid(pid, &status, WUNTRACED); // Keeping track of the child process, it should turn into a parent process
        } while(!WIFEXITED(status) && !WIFSIGNALED(status)); // Making sure the child is created and changed correctly
    }
    return 1; // Signal to the user that they can enter input again
}

// Function delcorations for builtin shell commands
int lsh_cd(char **args);
int lsh_help(char **args);
int lsh_exit(char **args);

// List of builtin commands, with their corresponding functions
char *builtin_str[] = {
    "cd",
    "help",
    "exit"
};

int (*builtin_func[]) (char **) = {
    &lsh_cd,
    &lsh_help,
    &lsh_exit
};

int lsh_num_builtins(){
    return sizeof(builtin_str) / sizeof(char *);
}

// Builtin function implementation
// This changes the directory
int lsh_cd(char **args){
    if(args[1] == NULL){
        // Throw an error, we expected another argument
        fprintf(stderr, "lsh: expected argument to \"cd\"\n");
    }
    else{
        if(chdir(args[1]) != 0){
            // Making sure we changed dirrectory
            perror("lsh");
        }
    }
    // We changed directory
    return 1;
}

// This is the help page
int lsh_help(char **args){
    int i;
    printf("Jon Richardson's LSH\n");
    printf("---\n");
    printf("The following are built in:\n");

    for(i = 0; i < lsh_num_builtins(); i++){
        printf("    %s\n", builtin_str[i]);
    }
    printf("Use the man command for information on other programs.\n");
    return 1;

}

// This exits the shell
int lsh_exit(char **args){
    return 0;
}


// LEXICAL analasis
void lsh_loop(){
    // This is going to read a string of characters
    char *line; // This is going to read the line
    char **args; // This is going to break the line up into characters
    int status; // To keep track of the process

    do {
        printf("> ");
        line = lsh_read_line(); // Reading the characters and storing them in a "line"/buffer 
        args = lsh_split_line(line); // Reading the line and  storing the tokens
        status = lsh_execute(args); // This runs the shell

        free(line);
        free(args);
    } while(status);
}

int lsh_execute(char **args){
    int i;

    if(args[0] == NULL){
        // There was an empty command, so we can just return
        return 1;
    }
    // Otherwise we have to check what was passed
    for(int i = 0; i < lsh_num_builtins(); i++){
        if(strcmp(args[0], builtin_str[i]) == 0){
            return (*builtin_func[i])(args);
        }
    }
    return lsh_launch(args);
}

// Need to take the input, parse it and break it up into tokens. Then make sure the tokens come in the correct order
int main(int argc, char *argv[] ){
    // Load files here

    // Start the loop here
    lsh_loop();
    
    // Shutdown/cleanup
    
    return EXIT_SUCCESS;
}
