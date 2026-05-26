#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <unistd.h>
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
