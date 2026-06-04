#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

// using linked list to keep track of back ground processes precisely and effectively without having to allocate memory
typedef struct background_job {
	pid_t pid;
	char cmd[2000];
	struct background_job *next; // points to the next background job
} background_job;

background_job *background_head = NULL;
volatile pid_t running = 0; // for keeping track of foreground process being executed so when ctrl c is entered we know if there is a process to stop

// when a background process is created and executed, this function is called to at it to a node part of the linked list, including its pid and cmd. it is dynamically allocated in memory and will be freed when it terminates 
void add_bg_job(pid_t pid, char *cmd) {
	background_job *new_bg = malloc(sizeof(background_job));
	new_bg->pid = pid;
	strcpy(new_bg->cmd, cmd);
	new_bg->next = background_head; // LIFO;; newest process is the head. older ones are further back
	background_head = new_bg;
}

// check the status of background processes to find zombie processes
void check_bg() {
	pid_t pid;
	int status;
	// find exited child processes to clear them off the background list, if there is none then do not wait
	while ((pid = waitpid(-1, &status, WNOHANG)) > 0) { // waitpid returns dead childs pid
		// start iterating through the linked list to find the address of the terminated background process
		background_job *cur = background_head; 
		background_job *prev = NULL;

		while (cur != NULL) {
			if (cur->pid == pid) { // make the pointer skip this node, removing the already terminated process
				if (prev == NULL) {
					background_head = cur->next;
				}	
				else {
					prev->next = cur->next;
				}
				printf("%d: %s has terminated.\n", cur->pid, cur->cmd);
				free(cur); // free the node from memory
				break;
			}
			// move to the next node until we reach NULL
			prev = cur;
			cur = cur->next;
		}
	}
}

// built-in command: bglist
void print_bglist() {
	background_job *cur = background_head;
	int count = 0; // initialize a counter that goes up by one every time we loop through a process node
	while (cur != NULL) { // check every node in the linked list, if NULL then we reached the end
		printf("%d: %s\n", cur->pid, cur->cmd);
		count++;
		cur = cur->next;
	}
	printf("Total background jobs: %d\n", count);
}

// ctr c handler
void sigint_handler(int signal) {
	if (running > 0) {
		kill(running, SIGINT); // kills running foreground process
	}
	else {
		printf("\n"); // if there is no fg process, create newline
		rl_on_new_line(); // move to new prompt line
		rl_replace_line("", 0); // clears currently typed out command
		rl_redisplay(); // prints out new prompt on newline as we move down
	}
}


int main()
{	
	signal(SIGINT, sigint_handler); // calls signint_handler when ^C is received

	char* username;
	char hostname[500];
	char cwd[2000];
	char buffer[3000];

	int bailout = 0;
	
	gethostname(hostname, sizeof(hostname));
	username = getlogin();

	while (!bailout)
	{
		check_bg();

		getcwd(cwd,sizeof(cwd));

		snprintf(buffer, sizeof(buffer), "%s@%s: %s > ", username, hostname, cwd); // prompt

		char* reply = readline(buffer); // takes user input
		
		// if nothing is entered, jumps to new input line and prints out the prompt again
		if (reply == NULL) {
			printf("\n");
			break;
		}
		
		// add input to history
		if (strlen(reply) > 0) {
			add_history(reply);
		}
		

		char arg_count = 0;
		size_t capacity = 10; // big guess for most command lines number of arguments :D
		char **args = (char**) malloc(capacity * sizeof(char*)); // allocate memory for tokenized input arguments

		char *token = strtok(reply, " \t\n\t"); // first argument

		// tokenize input 
        	    while (token != NULL) {
     	            	args[arg_count] = token; // keeps adding arguments to the array
			arg_count++;
			
			// if the amount of arguments is bigger than 8, reallocate the memory for larger input
			if (arg_count >= capacity - 1) {
				capacity = capacity * 2;
				args = realloc(args, capacity * sizeof(char*));
			}
                        token = strtok(NULL, " \t\n\r");; // continue tokenizing
                }
                args[arg_count] = NULL; //null-terminated for execvp when we reach the end of the arguments array

		// ignore empty input
		if (arg_count == 0) {
			free(reply);
			continue;
		}

		if (strcmp(args[0], "cd") == 0) {
			char *destination = args[1];
			char extra_path[1024];
			if (destination == NULL || strcmp(destination, "~") == 0) {
				destination = getenv("HOME"); // if no arguement return home
			}
			else if (destination[0] == '~') {
				snprintf(extra_path, sizeof(extra_path), "%s%s", getenv("HOME"), destination + 1);
				destination = extra_path;
			}

			if (chdir(destination) != 0) {
				perror("cd");
			}
		}
		else if (strcmp(args[0], "bglist") == 0) {
			print_bglist();
		}
		else {
			int bg = 0;
			int bg_start_index = 0;
			
			// check if its a background process
			if (strcmp(args[0], "bg") == 0) {
				bg = 1;
				bg_start_index = 1;
			}
			
			// fork a child so command can be executed by a child process while preserving parent's memory 
			pid_t pid = fork();

			if (pid < 0) {
				perror("fork() failed");
			}

			// child process
			else if (pid == 0) {
				if (bg) {
					// im assumming background processes arent affected by ^C
					setpgid(0, 0);
				}
				else {
					// restore ^C behavior if its foreground. when in the child's process the pid is set to 0, therefore if it inherits signal(SIGINT, signal_handler) and then it receives ctrl c then it wont kill() its process as pid is set to 0.
					signal(SIGINT, SIG_DFL);
				}

				// if bg is stated, skip the first argument 
				if (execvp(args[bg_start_index], &args[bg_start_index]) < 0) {
					printf("%s: No such file or directory\n", args[bg_start_index]);
					exit(1);
				}					
			}
			// parent process
			else {
				if (bg) {
					char *full_cmd = malloc(capacity * sizeof(char));

					if (full_cmd == NULL) {
						perror("malloc failed");
						exit(1);
					}

					full_cmd[0] = '\0'; // initialize it as an empty string for strcat

					for (int i = bg_start_index; args[i] != NULL; i++) {
						strcat(full_cmd, args[i]);
							if (args[i+1] != NULL) {
								strcat(full_cmd, " ");
							}
					}
					add_bg_job(pid, full_cmd);
				}
				else {
					running = pid; // assign the pid of the fg running process
					waitpid(pid, NULL, 0); // wait for fg process
					running = 0; // fg process finished
				}	
			}
		}
		
		// ^D or EOF
		if (reply == NULL)
		{
			bailout = 1;
		}
		free(reply);
	}
	printf("\n");
	return 0;
}
