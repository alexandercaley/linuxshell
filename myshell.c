/****************************************************************
 * Name        :  Alexander Caley                               *
 * Class       :  CSC 415                                       *
 * Date        :  03/19/19                                      *
 * Description :  Writing a simple bash shell program           *
 *                that will execute simple commands. The main   *
 *                goal of the assignment is working with        *
 *                fork, pipes and exec system calls.            *
 ****************************************************************/

#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

/* CANNOT BE CHANGED */
#define BUFFERSIZE 256
/* --------------------*/
#define PROMPT "myShell >> "
#define PROMPTSIZE sizeof(PROMPT)

 //Needed function prototypes
 void execute(char*[], int);
 void executeCommands(char*[], int, int);
 void backgroundProcess(char*[], int);
 void redirectOutput(char*[], int);
 void redirectOutput_append(char*[], int);
 void redirectInput(char*[], int);
 void pipeCommand(char*[], int);
 int indexOfArg(char*[], char*, int);


//Clear page and present program
void init(){
  system("clear");
	printf("\n\n******************************");
	printf("\n\t~*Alexander's Shell*~");
	printf("\n\t   ~*CSC 415*~");
	printf("\n\n******************************\n\n\n\n\n\n");
	sleep(1);
}

//creates new processes to be executed
void execute(char **myargv, int background) {

  //new process
  pid_t pid = fork();
  if(pid < 0) {
    //error
    printf("Creating new process was unsuccessful.");
  } else if(pid == 0) {
    //new child process
    if(background) {
			setpgid(pid, 0);
		}
    execvp(myargv[0], myargv);
    //error check
    fprintf(stderr, "Error: failed to execute %s.\n", myargv[0]);
		exit(-1);
  } else {
    //parent process waits for child process
    waitpid(pid, NULL, background ? WNOHANG : 0);
  }
}

//commands based on user input
void executeCommands(char **myargv, int myargc, int background) {
  if(indexOfArg(myargv, "&", myargc) >= 0) {
		backgroundProcess(myargv, myargc);
	} else if(indexOfArg(myargv, "|", myargc) >= 0) {
		pipeCommand(myargv, myargc);
	} else if(indexOfArg(myargv, "<", myargc) >= 0) {
		redirectInput(myargv, myargc);
	} else if(indexOfArg(myargv, ">", myargc) >= 0) {
		redirectOutput(myargv, myargc);
	} else if(indexOfArg(myargv, ">>", myargc) >= 0) {
    redirectOutput_append(myargv, myargc);
  } else {
		execute(myargv, background);
	}
}

//allow running of background processes
void backgroundProcess(char **myargv, int myargc) {
  int index = indexOfArg(myargv, "&", myargc);
  if(index != myargc - 1) {
    //check for &
    fprintf(stderr, "Error: '&' should be placed at the end of command.\n");
  } else {
    //execute
    myargv[myargc - 1] = NULL;
    --myargc;
    executeCommands(myargv, myargc, 1);
  }
}

void redirectOutput(char **myargv, int myargc) {

  //get index of >
  int index = indexOfArg(myargv, ">", myargc);
  if(myargc < 3 || index != myargc - 2) {
    //check for input error
    fprintf(stderr, "Error: '>' is incorrectly placed.\n");
    return;
  }

  //initialize the left args and agrc
  char *left_args[index + 1];
  int left_argc = index;
  for(int i = 0; i < left_argc; i++) {
    left_args[i] = myargv[i];
  }
  left_args[left_argc] = NULL;

  //duplicate stdout file descriptor
  int stdout_dup = dup(STDOUT_FILENO);
  //open file for write
  int output_file = open(myargv[myargc - 1], O_RDWR | O_CREAT | O_TRUNC, 0644);
  if(output_file < 0) {
    //check if there is end of file
    fprintf(stderr, "Error: failed to open or create file %s.\n", myargv[myargc - 1]);
  } else {
    //duplicate stdout file descriptor to output file
    dup2(output_file, STDOUT_FILENO);
    executeCommands(left_args, left_argc, 0);
    close(output_file);
    //restore stdout file descriptor
    dup2(stdout_dup, STDOUT_FILENO);
  }
}

void redirectOutput_append(char **myargv, int myargc) {

  //get index of >>
  int index = indexOfArg(myargv, ">>", myargc);
  if(myargc < 3 || index != myargc - 2) {
    //check if >> is placed correctly
    fprintf(stderr, "Error: '>>' is incorrectly placed.\n");
    return;
  }

  //initialize the left args and agrc
  char *left_args[index + 1];
  int left_argc = index;
  for(int i = 0; i < left_argc; i++) {
    left_args[i] = myargv[i];
  }
  left_args[left_argc] = NULL;

  //duplicate stdout file descriptor
  int stdout_dup = dup(STDOUT_FILENO);
  //open file for append
  int output_file = open(myargv[myargc - 1], O_RDWR | O_CREAT | O_APPEND, 0644);
  if(output_file < 0) {
    //check if there is EOF
    fprintf(stderr, "Error: failed to open or create file %s.\n", myargv[myargc - 1]);
  } else {
    //duplicate stdout file descriptor to output file
    dup2(output_file, STDOUT_FILENO);
    executeCommands(left_args, left_argc, 0);
    close(output_file);
    //restore stdout file descriptor
    dup2(stdout_dup, STDOUT_FILENO);
  }
}

void redirectInput(char **myargv, int myargc) {

  //get index of <
  int index = indexOfArg(myargv, "<", myargc);
  if (myargc < 3 || index != myargc - 2) {
    //check if < is placed correctly
    fprintf(stderr, "Error:'<' is incorrectly placed.\n");
		return;
  }

  //initialize the left args and agrc
  char *left_args[index + 1];
	int left_argc = index;
	for(int i = 0; i < left_argc; i++) {
		left_args[i] = myargv[i];
	}
	left_args[left_argc] = NULL;

  //duplicate stdin file descriptor
  int stdin_dup = dup(STDIN_FILENO);
  //open file for read
  int input_file = open(myargv[myargc - 1], O_RDONLY);
  if(input_file < 0) {
    //check if there is EOF
    fprintf(stderr, "Error: failed to open file %s.\n", myargv[myargc - 1]);
  } else{
    //duplicate stdin file descriptor to input file
    dup2(input_file, STDIN_FILENO);
    executeCommands(left_args, left_argc, 0);
    close(input_file);
    //restore stdin file descriptor
    dup2(stdin_dup, STDIN_FILENO);
  }
}

void pipeCommand(char **myargv, int myargc) {

  //initialize the variables
  char *left_args[BUFFERSIZE];
  int left_argc;
  int j;
  int prev_pipe_index = -1;
  int fds[2];
  int fd_in  = -1;
  int fd_out = -1;
  pid_t pid;

  //loop through myargv until a pipe found
  for(int i = 0; i < myargc; i++) {
    if(strcmp(myargv[i], "|") == 0 || i == myargc - 1) {
      //process left command
      left_argc = 0;
      if(prev_pipe_index > 0) {
        //update j
        j = prev_pipe_index + 1;
      } else {
        //encounter first pipe
        j = 0;
      }

      if(i < myargc - 1) {
        //update prev_pipe_index
        prev_pipe_index = i;

        //populate the left args
        for(/*initialized*/; j < i; j++) {
          left_args[left_argc++] = myargv[j];
        }
        left_args[left_argc] = NULL;

        //create new pipe
        pipe(fds);
        //set fd_out to be the write end of pipe
        fd_out = fds[1];
      } else {
        //populate the left args
        for(/*initialized*/; j <= i; j++) {
          left_args[left_argc++] = myargv[j];
        }
        left_args[left_argc] = NULL;

        //set fd_out to be -1 when final input is read
        fd_out = -1;
      }

      //create new process
      pid = fork();
      if(pid < 0) {
        //creating new process fails
        printf("Unsuccessful creating process.");
      } else if(pid == 0) {
        //child process execute commands
        if(fd_in != -1 && fd_in != 0) {
          //duplicate stdin to read end of pipe when not first command
          dup2(fd_in, STDIN_FILENO);
          close(fd_in);
        }

        if(fd_out != -1 && fd_out != 1) {
          //duplicate stdout to write end of pipe when not last command
          dup2(fd_out, STDOUT_FILENO);
          close(fd_out);
        }

        //execute commands
        executeCommands(left_args, left_argc, 0);
        exit(0);
      } else {
        //parent process waits for child process
        waitpid(pid, NULL, 0);
        close(fd_in);
        close(fd_out);
        fd_in = fds[0];
      }
    }
    //end of for loop
  }
}

int indexOfArg(char **myargv, char *arg, int myargc) {

	//return the index of arg in myargv
	for(int i = 0; i < myargc; i++) {
		if(strcmp(myargv[i], arg) == 0) {
			return i;
		}
	}
  //return -1 if arg is not found
	return -1;
}

//main program starts from here
int main(int myargc, char **myargv) {

  //initialize the variables
  char input_buf[BUFFERSIZE];
	char *token;

  signal(SIGCHLD, SIG_IGN);

  //read user input until user enter "exit"
  init();
  printf(PROMPT);
  while(fgets(input_buf, BUFFERSIZE, stdin) != NULL) {
    //reset myargc
    myargc = 0;
    input_buf[strlen(input_buf) - 1] = '\0';

    //seperate user input into tokens and add them to myargv
    //store all text in myargv[1] if cd is the first token
    token = strtok(input_buf, " ");
    if (strcmp(token, "cd") == 0){
      myargv[myargc++] = token;
      token = strtok(NULL, "\n");
      //printf("%s\n", token);
      myargv[myargc++] = token;
    }
    else{
      while(token != NULL) {
          myargv[myargc++] = token;
          //printf("%s\n", token);
          token = strtok(NULL, " ");
      }
    }

    //execute commands according to user input
    if(myargc > 0) {
      //check if user enter 'exit'
      if(strcmp(myargv[0], "exit") == 0) {
        break;
      }
      //uses built in function to change directory
      else if (strcmp(myargv[0], "cd") == 0){
        chdir(myargv[1]);
      }
      //clears previous outputs   !!!having some issues!!!
      else if (strcmp(myargv[0], "clear") == 0){
        system("clear");
      }
      else {
        //execute commands
        myargv[myargc] = NULL;
        executeCommands(myargv, myargc, 0);
      }
    }
    //end of loop
    printf(PROMPT);
  }
  return 0;
}
