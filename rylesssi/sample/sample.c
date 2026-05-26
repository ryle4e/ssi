#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

typedef struct background_job {
	pid_t pid;
	char cmd[1024];
	struct background_job *next;
} background_job;

background_job *background_head = NULL
volatile sig_atomic_t running = 0; //if there is a foreground process running



void add_bg_job(pid_t pid, char *cmd) {
	background_job *new_bg = malloc(sizeof(background_job));
	new_bg->pid = pid;
	strcpy(new_bg->cmd, cmd);
	new_bg->next = background_head;
	background_head = new_bg;
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

		if (!strcmp(reply, "bye"))
		{
			bailout = 1;
		}
		else
		{
			printf("\nYou said: %s\n\n", reply);
		}

		free(reply);
	}
	printf("Bye Bye\n");
	return 0;
}
