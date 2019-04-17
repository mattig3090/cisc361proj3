//Anthony Sette
#include "get_path.h"

int pid;
int sh( int argc, char **argv, char **envp);
void exitshell();
char *which(char *command, struct pathelement *pathlist);
char *where(char *command, struct pathelement *pathlist);
void cd(char *pth, char *prev);
void printwd();
void list(char *dir);
void printpid();
void kill_process(char **args, int count);
void printenv(char **envp);
void printenvvar(char *env_var);
void newline(char *string);
int search(char *command, char **parameters);
int wildcardhelper(char *check_string);
void set_env(char *envname, char *envval); //setenv is an internal function so I used set_env
void freepathlist(struct pathelement *head);
void sigintHandler(int sig_num);
void signalSTPHandler(int sig_num);
char *list_wildcard (char *dir, char *wildcard);
char *args_to_string(char **args, int arg_count);

#define PROMPTMAX 32
#define MAXARGS 10