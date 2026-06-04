Assignment 1

Course: CSC360
Professor Wenjun Yang
2 June 2026

Rylee Ngo
V01068147

FEATURES:

1. Shell Prompt: Displays "username@hostname: cwd >"
- Uses getcwd() to get current working environment
- Uses gethostname() to get host name
- Uses getenv() for HOME directory
- Users getlogin() for user
- Updates correctly based on directory change requests (cd,...)

2. Takes input from user using readline
- If nothing is typed in, prints newline to mimic the behaviour of bash
- Saves input in history using add_history()
- Tokenizes input into arguements for command execution progress

3. Built-in commands:
a. cd:
- Works with "..", ".", ~
- Goes to home dir if only "cd" is received
- Support multiple arguements in the form "dir1/dir2/..."
b. bglist:
- Uses linked list for keeping track of background processes
- Returns number of bg processes as well as their pids
- Memory is freed when jobs terminate

4. External commands:
- Fork child processes and execute commands after tokenization using execvp(). 
- Background jobs are assigned their own process groups using setgpid(0,0) so they don't get affected by ^C
- Clears up terminated child processes using waitpid(-1, &status, WNOHANG) in a loop through all the background processes
- Prints termination message immediately when a background process terminates

5. Signal handling:
- Sigint_handler() intercepts ^C via signal()
- If an empty input is received, clears the line and jumps to newline with the same prompt using rl_on_new_line(), rl_replace_line(), rl_redisplay()
- During a foreground process execution, if ^C is received, terminates the foreground processes:
	- The handler sends the SIGINT signal via kill() to child process that is tracked using the running variable
	- Still keeps the parent shell and background jobs running
