#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_ARGS 256

typedef struct background_job {
	pid_t pid;
	char cmd[1024];
	struct background_job *next;
} background_job;

background_job *background_head = NULL;
volatile sig_atomic_t running = 0; //turns into a positive number if there is a foreground process running



void add_bg_job(pid_t pid, char *cmd) {
	background_job *new_bg = malloc(sizeof(background_job));
	new_bg->pid = pid;
	strcpy(new_bg->cmd, cmd);
	new_bg->next = background_head;
	background_head = new_bg;
}

void check_bg() {
	pid_t pid;
	int status;
	// find exited child processes to clear them off the background list
	while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
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
			prev = cur;
			cur = cur->next;
		}
	}
}

void print_bglist() {
	background_job *cur = background_head;
	int count = 0;
	while (cur != NULL) {
		printf("%d: %s\n", cur->pid, cur->cmd);
		count++;
		cur = cur->next;
	}
	printf("Total background jobs: %d\n", count);
}

void sigint_handler(int signal) {
	if (!running) {
		printf("\n");
		rl_on_new_line();
		rl_replace_line("", 0);
		rl_redisplay();
	}
}


int main()
{
	char* username;
	char hostname[256];
	char cwd[1024];
	char buffer[2048];

	int bailout = 0;
	
	gethostname(hostname, sizeof(hostname));
	username = getlogin();

	while (!bailout)
	{
		check_bg();

		getcwd(cwd,sizeof(cwd));

		snprintf(buffer, sizeof(buffer), "%s@%s: %s > ", username, hostname, cwd);

		char* reply = readline(buffer);

		if (reply == NULL) {
			printf("\n");
			break;
		}
		
		if (strlen(reply) > 0) {
			add_history(reply);
		}

		char *args[MAX_ARGS];
		int arg_count = 0;
		char *token = strtok(reply, " \t\n");

		while (token != NULL && arg_count < MAX_ARGS - 1) {
			args[arg_count++] = token;
			token = strtok(NULL, " \t\n");
		}
		args[arg_count] = NULL; //null-terminated for execvp

		if (arg_count == 0) {
			free(reply);
			continue;
		}

		if (strcmp(args[0], "cd") == 0) {
			char *destination = args[1];
			if (destination == NULL || strcmp(destination, "~") == 0) {
				destination = getenv("HOME"); // if no arguement return home
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
			if (strcmp(args[0], "bg") == 0) {
				bg = 1;
			}
			pid_t pid = fork();
			if (pid < 0) {
				perror("fork() failed");
			}
			// child process
			else if (pid == 0) {
				if (bg) {
					setpgid(0, 0);
				}
				else {
					signal(SIGINT, SIG_DFL);
				}
				if (execvp(args[1], &args[1]) < 0) {
					printf("%p: No such file or directory\n", args[1]);
					exit(1);
				}					
			}
			// parent process
			else {
				running = 1;
				waitpid(pid, NULL, 0);
				running = 0;
			}	
			
		}

		if (!strcmp(reply, "^D"))
		{
			bailout = 1;
		}
		free(reply);
	}
	printf("Bye Bye\n");
	return 0;
}
