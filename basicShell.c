#include "fcntl.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "sys/wait.h"
#include "unistd.h"
#include "ctype.h"
#include "stdbool.h"

void trim_whitespace(char *str);

#define LINE_SIZE 1024      // max can be 1024 bytes
#define BUFFER_SIZE 128
// #define MAX_ARGS 20 

// reads a single line from the terminal returns a pointer
// to the character array or NULL if an error is encountered

/*Further explanation about why returning a char pointer ; a C string is represented as a 
sequence of characters, and it is typically accessed through a pointer to the first character 
in that sequence. Therefore, when you have a char * pointer in C, it can be used to point to 
the beginning of a character sequence, which effectively makes it a string.*/
char *readLine(){
    int lineLength;
    char *line = (char *) malloc(sizeof(char) * LINE_SIZE);
    char c;
    int count = 0;
    int cap = LINE_SIZE;

    while(1){
        c = getchar();
        if(c == '\n' || c == EOF){
            line[count] = '\0';
            return line;
        }

        if(count == cap){
            cap += LINE_SIZE;
            line = (char *) realloc(line, cap);
            if(line == NULL){
                fprintf(stderr, "readLine: cannot allocate memory\n");
                return NULL;
            }
        }
        line[count++] = c;
    }
    return line;
}


// obtains user input line and splits up into multiple commands separated by pipes
// returns a double pointer to a string array, each entry containing the string 
// associated with a single command in the pipe
char **splitLine(char *line, int *numOfCommands){

    int numArgs=0; // counter for the number of arguments found

    //create copy of input line to avoid modifying original with strtok later
    char inputLine[strlen(line)+1];
    strcpy(inputLine,line);       

    //count the number of pipes '|' to determine the number of commands
    for (int i=0; i<strlen(line);i++){
        if (inputLine[i]=='|'){
            numArgs++;
        }
    }
    numArgs++; // account for last command after final '|' 

    //allocate mem for array of command strings
    char **arguments=(char **)malloc((numArgs+1)*sizeof(char*));

    //split the input line into commands separated by '|'
    char *token=strtok(inputLine,"|");
    int i=0;
    while (token!=NULL){
        //allocate memory for each command
        arguments[i]=(char*)malloc(1024*sizeof(char));
        trim_whitespace(token);

        //copy the trimmed command into the arguments array
        strcpy(arguments[i],token);
        i++;
        token=strtok(NULL,"|");
    }
    //null-terminate the array of commands
    arguments[i]=NULL;

    //store the number of commands found
    *numOfCommands=numArgs;

    //return array of command strings
    return arguments;
}


// obtains a single string representing a command
// parses the words of the command into tokens and strips
// any single or double quotes around any one word
// NOTE: arguments surrounded by double or single quotes
// are considered one argument or one token; meaning:
// cmd1 "input string" has two tokens, not 3.
char **parseCommand(char *command, int *numOfWords){

    char **commands = malloc(100* sizeof(char*));  // Allocate space for up to 100 commands
    char *token = malloc(100 * sizeof(char)); // allocate temp storage for building each token
    int count = 0; // number of tokens found
    int tokenIndex = 0; // current position in the token being built
    bool inDoubleQuote = false; //used to track if current character is inside a double quote


    //iterate over each character in the command
    for (int i = 0; i < strlen(command) + 1; i++) {
        char currentChar = command[i];

        // Toggle the inDoubleQuote flag if a double quote is encountered
        if (currentChar == '\"') {
            inDoubleQuote = !inDoubleQuote;
            continue;  // Skip adding the double quote to the token
        }

        // When we encounter a space or arrive at the end of the command
        // if not within quotes, add the token to the commands array
        if ((currentChar == ' ' && !inDoubleQuote) || currentChar == '\0') {
            token[tokenIndex] = '\0';  // Null-terminate the token
            if (tokenIndex > 0) {  // Ignore empty tokens
                commands[count] = strdup(token);  // Duplicate the token to commands array
                count++;
                tokenIndex = 0;  // Reset for the next token
            }
            if (currentChar == '\0'){
                break;  // If at the end of the command, stop parsing
            }
        } else {
            token[tokenIndex++] = currentChar;  // Add the character to the current token
        }
    }

    free(token);  // Free the temporary token storage
    commands[count] = NULL;  // Null-terminate the array of commands
    *numOfWords = count;

    return commands;
}


// obtains the tokens and number of tokens associated with a single command
// also obtain the in and out file descriptors if successful, this function
// will execute the function given, reading from inFD and writing to outFD
// and won't return. If not, it will return 1. This function is also 
// responsible for handling redirects and the handling of background processes
int shellExecute(char *tokens[], int numOfTokens, int inFD, int outFD) {
    char output_file[100];
    char input_file[100];
    output_file[0] = '\0'; // initialize output files
    input_file[0] = '\0';  // initialize input files

    // Find the input file and output file
    for (int i = 0; i < numOfTokens; i++) {
        // If there's still at least one more token left
        if (i + 1 < numOfTokens) {
            // If we encounter "<", then the next token is the input redirection file
            if (strcmp(tokens[i], "<") == 0) {
                strcpy(input_file, tokens[i + 1]);
            }
            // If we encounter ">", then the next token is the output redirection file
            else if (strcmp(tokens[i], ">") == 0) {
                strcpy(output_file, tokens[i + 1]);
            }
        }
    }

    // Open the input file if specified - use as input
    if (input_file[0] != '\0') {
        inFD = open(input_file, O_RDONLY);
        if (inFD == -1) {
            perror("Failed to open input file");
            return -1;
        }
    }

    // Open the output file if specified - use as output
    if (output_file[0] != '\0') {
        outFD = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0777);
        if (outFD == -1) {
            perror("Failed to open output file");
            return -1;
        }
    }

    // Prepare argument vector for execvp
    char *pargv[20]; // array of pointers to strings for arguments to execute

    // Iterate through the token list
    int j = 0;
    for (int i = 0; i < numOfTokens; i++) {
        if (strcmp(tokens[i], ">") == 0 || strcmp(tokens[i], "<") == 0) {
            // If it equals "<" or ">", then stop for the argument
            break;
        }
        // Otherwise, copy over the argument
        pargv[j] = (char *)malloc((strlen(tokens[i]) + 1) * sizeof(char));
        strcpy(pargv[j], tokens[i]); // copy over the argument
        j++;
    }
    pargv[j] = '\0'; // add null terminator for execvp

    // Fork a new process to execute the command
    int pid = fork();
    if (pid == 0) { // Child process
        // Redirect input and output as necessary
        if (inFD != STDIN_FILENO) {
            dup2(inFD, STDIN_FILENO); // Duplicate input file descriptor to STDIN
            close(inFD); // Close original input file descriptor
        }
        if (outFD != STDOUT_FILENO) {
            dup2(outFD, STDOUT_FILENO); // Duplicate output file descriptor to STDOUT
            close(outFD); // Close original output file descriptor
        }
        // Execute the command
        execvp(pargv[0], pargv);
        // If execvp returns, it has failed
        perror("execvp failed");
        exit(1);
    } 

    // If the last token is "&", the command is a background process
    if (strcmp(tokens[numOfTokens - 1], "&") == 0) {
        printf("[%d] background:\n", pid);
    } 
    else {
        // Parent process waits for the child to complete
        waitpid(pid,NULL,0);
    }

    return 0; // Successful execution
}

int main(){
    char *line = NULL;              // line read from terminal
    /*NOTE: this line is not representitive of a concatenated argv since
    it may include symbols such as such as '|', '&', '<' or '>'*/

    char **commandList = NULL;      // list of commands stored in a string array
    int numOfCommands;              // number of commands in a single line
    /*NOTE: a single entry within the commandList is not argv since that entry
    may contain symbols such as '&', '<' or '>'*/

    char **tokens = NULL;           // tokens associated with a single command
    int numOfTokens;                // number of tokens associated with a single command
    /*NOTE: these are not yet argv and argc since they may contain non argument tokens
    such as '&', '<' or '>'.*/

    int status;                     // return status of an executed command


    while(1){
        line = readLine();
        commandList = splitLine(line, &numOfCommands);
        printf("\n>> ");

        // if a single command is parsed, it means there are
        // no pipes and we can just execute it normally
        if(numOfCommands == 1){
            tokens = parseCommand(line, &numOfTokens);
            // we check if the command given is a shell builtin
            // those have other APIs we can utilize without needing
            // to fork or do anything fancy
            if(strcmp(tokens[0], "exit") == 0){
                printf("Exiting...\n");
                break;
            }
            else if(strcmp(tokens[0], "cd") == 0){
                if (tokens[1] == NULL) {
                    fprintf(stderr, "cd: no path directory not specified\n");
                }
                else {
                    if (chdir(tokens[1]) != 0) {
                        fprintf(stderr, "cd: directory not found\n");
                    }
                }
            }
            else if(strcmp(tokens[0], "help") == 0){
                printf("SHELL HELP\n");
                printf("These are the built-in commands:\n");
                printf("    - help\n");
                printf("    - cd <path>\n");
                printf("    - exit\n");
                printf("Type man <command> to know about a command\n");
                printf("Type man to know about other commands\n");
                status = 0;
            }
            // if it's not a shell builtin, we execute it with standard 
            // in and out file descriptors
            else{
                status = shellExecute(tokens, numOfTokens, 0, 1);
            }
        }
        

        // if we have multiple commands and one or more pipes
        // then we have to construct and allocate the pipes
        // such that we can establish interprocess communication (IPC) 
        else if(numOfCommands > 1){
            // Create an array of pipes, one less than the number of commands, to connect each command
            int pipes[numOfCommands - 1][2];

            // Initialize each pipe
            for (int i = 0; i < numOfCommands - 1; i++) {
                pipe(pipes[i]);
            }

            // Iterate through each command separated by a pipe
            for (int i = 0; i < numOfCommands; i++) {
                char output_file[100]; // output redirection file
                char input_file[100];  // input redirection file
                output_file[0] = '\0'; // Initialize 
                input_file[0] = '\0';  // Initialize 

                // Parse the command into tokens, ignoring whitespace
                tokens = parseCommand(commandList[i], &numOfTokens);

                // Check for file redirection by looking for "<" and ">" tokens
                for (int j = 0; j < numOfTokens; j++) {
                    if (strcmp(tokens[j], "<") == 0) {
                        // If "<" is found, the next token is the input file
                        strcpy(input_file, tokens[j + 1]);
                    } else if (strcmp(tokens[j], ">") == 0) {
                        // If ">" is found, the next token is the output file
                        strcpy(output_file, tokens[j + 1]);
                    }
                }

                //create new process for the command execution 
                pid_t pid = fork();

                if (pid == 0) { // Child process

                    //need to check first command, since it might read from an input file instead of a pipe
                    if (i==0){
                        //check if a input redirection file is there
                        if (input_file[0]!='\0'){
                            //if yes, create file descriptor for input file
                            int inFD = open(input_file, O_RDONLY);
                            // process will now read from input file instead of STDIN
                            dup2(inFD, STDIN_FILENO); 
                            close(inFD);
                        }
                    }
                    //if not the first command
                    else if (i>0){
                        //for all remaining commands, read from the end of the previous pipe as STDIN
                        //this lets output of last command be passed in as input to this command
                        dup2(pipes[i-1][0],STDIN_FILENO);
                    }

                    //for all commands except for the last
                    if (i < numOfCommands - 1) {
                        //output of the command should be directed to write end of current pipe
                        //will be read by next command
                        dup2(pipes[i][1], STDOUT_FILENO);
                    } 
                    // for the last command, we check if an output_file has been specified for redirection
                    else if (output_file[0] != '\0') {
                        //if yes, redirect output to the output file
                        int outFD = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0777);
                        dup2(outFD, STDOUT_FILENO);
                        close(outFD);
                    }

                    // Close all pipe file descriptors in the child process
                    for (int j = 0; j < numOfCommands - 1; j++) {
                        close(pipes[j][0]);
                        close(pipes[j][1]);
                    }

                    // Prepare the argument vector for execvp
                    char *pargv[20]; // Argument vector for execvp
                    int m = 0;
                    //iterate through the tokens of the command 
                    for (int i = 0; i < numOfTokens; i++) {
                        if (strcmp(tokens[i], ">") == 0 || strcmp(tokens[i], "<") == 0) {
                            // Skip redirection delimiters
                            break;
                        }
                        // Copy over the argument
                        pargv[m] = (char *)malloc((strlen(tokens[i]) + 1) * sizeof(char));
                        strcpy(pargv[m], tokens[i]);
                        m++;
                    }
                    pargv[m] = NULL; // Null-terminate the argument vector

                    // Execute the command
                    execvp(pargv[0], pargv);
                    perror("execvp"); // If execvp returns, it has failed
                    exit(1); // Exit
                } else if (pid < 0) {
                    perror("fork"); // Fork failed
                    exit(1);
                }
            }

            // Close all pipes in the parent process
            for (int i = 0; i < numOfCommands - 1; i++) {
                close(pipes[i][0]);
                close(pipes[i][1]);
            }

            // Wait for all child processes to finish
            // ensure that shell doesn't proceed to execute other scripts
            // until after current pipeline is done
            for (int i = 0; i < numOfCommands; i++) {
                wait(NULL);
            }

        }


        // if you used dynamic mmeory allocation (which I highly recommend in this
        // instance) this code will deallocate it
        if(tokens){
            free(tokens);
            tokens = NULL;
        }
        if(line){
            free(line);
            line = NULL;
        }
        if(commandList){
            free(commandList);
            commandList = NULL;
        }
    }
    // IF for whatever reason we left the while loop earlier - make sure everything is fine 
    if(tokens) free(tokens);
    if(line) free(line);
    if(commandList) free(commandList);
    
    return 0;
}


//trim leading whitespace characters from given string
void trim_whitespace(char *str) {
    int i = 0, j = 0;
    
    //skip over leading whitespace characters
    while (isspace((unsigned char)str[i])) {
        i++;
    }
    //copy non-whitespace characters to the beginning of the string
    while (str[i] != '\0') {
        str[j++] = str[i++];
    }

    //terminate string after removing leading whitespace
    str[j] = '\0';
}


// **********  TESTING ********** 
// worked: 
    // cat testfile.txt | wc > newfile.txt 
    // wc < tempfile.txt | cat & 
    // wc < tempfile.txt | cat
    // wc < tempfile.txt | cat | cat > finalfile.txt

// "cmd1 < inputFile.txt | cmd2 | cmd3 arg1 arg2 &"

// ls 
// ls -l 

// cat 
    // on its own 
    // cat testfile.txt | wc 
    // cat tempfile.txt | wc > FILEOUT &

// wc < tempfile.txt | cat | cat > finalFILE_new

// TESTING FOR ALL TYPES OF EDGE CASES // 

// Echo Command (echo):
    // echo "Hello, world!"
    // echo "The price is \$10."
    // echo ""
    // echo "Hello," "world!"
    // echo "Hello, world!" > output.txt
    // echo -e "Hello\tworld\n" ???? 


// List Files (ls):
    // ls
    // ls -a
    // ls -l
    // ls /path/to/directory -> ls /Users/mac/Desktop/OS/lab3_OS
    // ls -lh
    // ls -lt
    // ls -R
    // ls -l -a



// Word Count (wc):
    // wc filename.txt
    // wc -l filename.txt
    // wc -w filename.txt
    // wc -c filename.txt
    // echo "Hello, world!" | wc -w



// Concatenate (cat):
    // cat filename.txt
    // cat file1.txt file2.txt
    // cat file1.txt file2.txt > newfile.txt
    // cat -n filename.txt
    // ls | cat -n
    // cat file1.txt >> file2.txt -  ????


// Background Process (&):
    // Run a command in the background: sleep 10 &
    // Multiple background processes: sleep 5 & echo "Hello" & ls -l & - ??? 


// Pipes (|):
    // ls | grep "file"
    // ls | wc -l
    // ls | grep "file" | wc -l
    // ls | grep "file" > output.txt


// Redirection (<, >):
    // wc < input.txt
    // ls > filelist.txt
    // sort < input.txt > output.txt
    // cat < input.txt | grep "keyword"

