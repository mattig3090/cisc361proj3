//Anthony Sette
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <glob.h>
#include "sh.h" 

#define buf 1024 //buf size
#define max 2048 //max size

int sh( int argc, char **argv, char **envp )
{
  char *prompt = calloc(PROMPTMAX, sizeof(max));
  char *commandline = calloc(MAX_CANON, sizeof(char));
  char *command, *arg, *commandpath, *p, *pwd, *owd;
  char **args = calloc(MAXARGS, sizeof(char*));
  char **args_wild = calloc(buf, sizeof(char*));
  int uid, i, status, argsct, go = 1;
  struct passwd *password_entry;
  char *homedir;
  struct pathelement *pathlist;
  char * prev_enviornment = malloc(sizeof(prev_enviornment)); 

  // get user and pass then start in home dir
  uid = getuid(); //get uid
  password_entry = getpwuid(uid); //get pass
  homedir = password_entry->pw_dir; //start in home dir

  if ( (pwd = getcwd(NULL, PATH_MAX+1)) == NULL )
  {
    perror("getcwd");
    exit(2);
  }

  owd = calloc(strlen(pwd) + 1, sizeof(char));
  memcpy(owd, pwd, strlen(pwd));
  prompt[0] = ' '; prompt[1] = '\0'; //initial prompt

  // pathlist is defined
  pathlist = get_path();

  while ( go ){
    //used to handle ctrl z and ctrl c
    signal(SIGINT, sigintHandler); //sig init
    signal(SIGTSTP, signalSTPHandler); //sig stop

    //init cmdline
    char *cmdline = calloc(MAX_CANON, sizeof(char));

    //init and get current working directory (cwd)
    char cwd1[buf];
    getcwd(cwd1, sizeof(cwd1)); //get cwd
    printf("%s [%s]>", prompt, cwd1); //print/update cwd in shell

    //this handles ctrl d and lets user know that to quit they should use exit command.
    if (fgets(cmdline, MAX_CANON, stdin) == NULL) {
      printf("\nTo quit, please use the exit command: exit.\n");
    }

    newline(cmdline); //new line cmd
    int argsct = search(cmdline, args); //search cmd for args

/* Start of the checks for build in commands.*/

    // exit
    if (strcmp(args[0], "exit") == 0){ //arg is exit
      if (argsct > 1){ //incorrect number of many args
        fprintf(stderr, "Incorrect number of arguments for %s function.\n", args[0]);
      }
      else{ //execute
        printf("Executing built-in %s\n", args[0]);
        exitshell();
      }
    }

    // which
    else if (strcmp(args[0], "which") == 0){ //arg is which
      if(argsct == 1){ //incorrect number of many args
        fprintf(stderr, "Incorrect number of arguments for %s function.\n", args[0]);
      }
      else{ //execute
        int i = 1;
        for (i; i < argsct; i++){      //execute which for all args
          printf("Executing built-in %s\n", args[0]);
          char *cmd = malloc(sizeof(cmd));
          cmd = strdup(args[i]);
          char* c_path = which(cmd, pathlist); //run which
          printf("%s\n", c_path);
        }
      }
    }

    // where
    else if (strcmp(args[0], "where") == 0){ //arg is where
      if(argsct == 1){ //incorrect number of many args
        fprintf(stderr, "Incorrect number of arguments for %s function.\n", args[0]);
      }
      else{ //execute
        int i = 1;
        for (i; i < argsct; i++){    //execute where for all args
          printf("Executing built-in %s\n", args[0]);
          char *cmd = malloc(sizeof(cmd));
          cmd = strdup(args[i]);
          char* c_path = where(cmd, pathlist); //run where
          printf("%s\n", c_path);
        }
      }
    }

    // cd
    else if (strcmp(args[0], "cd") == 0){ //arg is cd
      char cwd[buf];
      getcwd(cwd,sizeof(cwd));
      if (argsct == 1){ //go home if no arg is given
        printf("Executing built-in %s\n", args[0]);
        strcpy(prev_enviornment, cwd);
        cd(getenv("HOME"), prev_enviornment);
      }
      else if(argsct == 2){ //go to directory given in arg2
        printf("Executing built-in %s\n", args[0]);
        char *temp = malloc(1024);
        strcpy(temp, prev_enviornment);
        if (chdir(args[1]) != -1 || args[1] == "-"){
          strcpy(prev_enviornment, cwd);
        }
        cd(args[1], temp);
      }
      else{ //incorrect number of many args
        fprintf(stderr, "Incorrect number of arguments for %s function.\n", args[0]);
      }
    }

    // pwd
    else if(strcmp("pwd", args[0]) == 0){ //arg is pwd
      if (argsct == 1){ //execute
        printf("Executing built-in %s\n", args[0]);
        printwd();
      }
      else{ //incorrect number of many args
        fprintf(stderr, "Incorrect number of arguments for %s function.\n", args[0]);
      }
    }

    // list
    else if (strcmp(args[0], "list") == 0){ //arg is list
      printf("Executing built-in %s\n", args[0]);
      if (argsct == 1){ //list current dir
        char cwd[buf];
        getcwd(cwd, sizeof(cwd));
        printf("%s: \n", cwd);
        list(cwd);
      }
      else{ //list all dirs given as args
        printf("Executing built-in %s\n", args[0]);
        int i;
        for(i = 1; i < argsct; i++){
          printf("%s: \n", args[i]);
          list(args[i]);
        }
      }
    }

    // pid
    else if (strcmp(args[0], "pid") == 0){ //arg is pid
      if(argsct > 1){ //incorrect number of many args
        fprintf(stderr, "Incorrect number of arguments for %s function.\n", args[0]);
      }
      else{ //execute
        printf("Executing built-in %s\n", args[0]);
        printpid();
      }
    }

    // kill
    else if (strcmp(args[0], "kill") == 0){ //arg is kill
      printf("Executing built-in %s\n", args[0]);
      if (argsct == 1){ //no process given
        fprintf(stderr,"Please enter what to kill.\n");
      }
      else { //execute
        kill_process(args, argsct);
      }
    }

    // prompt
    else if (strcmp(args[0], "prompt") == 0){ //arg is prompt
      if(argsct == 1){ //ask user for prompt since user did not give it
        printf("Executing built-in %s\n", args[0]);
        char *prom = malloc(sizeof(char));
        printf("%s", "input prompt prefix: ");
        fgets(prom, MAX_CANON, stdin);
        strtok(prom, "\n");
        strcpy(prompt,prom);
      }
      else if(argsct == 2){ //add second arg as prompt
        printf("Executing built-in %s\n", args[0]);
        strcpy(prompt, args[1]);
      }
      else{ //incorrect number of many args
        fprintf(stderr, "Incorrect number of arguments for %s function.\n", args[0]);
      }
    }

    // printenv
    else if (strcmp(args[0], "printenv") == 0){ //arg is printenv
      if (argsct == 1){ //print environment
        printenv(envp);
      }
      else if (argsct == 2){ //print environment variable
          printenvvar(args[1]);
      }
      else{ //incorrect number of many args
        fprintf(stderr, "Incorrect number of arguments for %s function.\n", args[0]);
      }
    }

    //setenv
    else if(strcmp(args[0], "setenv") == 0){ //arg is setenv
      printf("Executing built-in %s\n", args[0]);
      if (argsct == 1){
        printenv(envp);
      }
      else if (argsct == 2){ //set empty var
        set_env(args[1], " ");
        if (strcmp(args[1], "PATH") == 0){
          freepathlist(pathlist); //free path list
          pathlist = get_path();
        }
      }
      else if (argsct == 3){ //set var to value
        set_env(args[1], args[2]);
        if (strcmp(args[1], "PATH") == 0){
          freepathlist(pathlist); //free path list
          pathlist = get_path();
        }
      }
      else{ //error
        errno = E2BIG;
        perror("setenv");
      }
    }

    //If the argument is not built in, the process is forked. This allows the process to be run on the terminal through mysh.
    else{
      pid_t pid;
      char* p = args[0];
      if (access(args[0], F_OK) == -1){
        p = which(args[0], pathlist);
      }
      if (p == ""){   //if which doesnt find the process don't fork it.
        printf("\n");
      }
      else if (p != ""){
        pid = fork(); //fork it
        if (pid == 0){
          int status;
          if (strcmp("ls", args[0]) == 0 && wildcardhelper(args_to_string(args, argsct)) == 1){
            int l=1;
            int counter;
            char *ex_args = malloc(512);
            while(l <= argsct){
              // check if wildcard
              if(wildcardhelper(args[l]) == 1){
                char cwd[buf];
                getcwd(cwd, sizeof(cwd));
                char *wild_list = malloc(512);
                strcpy(wild_list, list_wildcard(cwd, args[l]));

                char *xx = malloc(strlen(wild_list) + strlen(ex_args) + 1);
                strcat(xx, ex_args);
                strcat(xx, " ");
                strcat(xx, wild_list);
                search(xx, args_wild);
                status = execvp(p, args_wild);
              }
              else {
                strcat(ex_args, " ");
                strcat(ex_args, args[l]);
                counter++;
              }
              l++;
            }
            search(ex_args, args_wild);
            execvp(p, args_wild);
          }

          // This is an overwrite for file allowing file * to be run.
          else if (strcmp("file", args[0]) == 0 && wildcardhelper(args_to_string(args, argsct)) == 1){
            int l=1;
            int counter;
            char *ex_args = malloc(512);
            while(l <= argsct){
              //check if arg has wild card 
              if(wildcardhelper(args[l]) == 1){
                char cwd[buf];
                getcwd(cwd, sizeof(cwd));
                //char *wild_list = malloc(strlen(list_wildcard(cwd, args[l])));
                char *wild_list = malloc(512);
                strcpy(wild_list, list_wildcard(cwd, args[l]));

                char *xx = malloc(strlen(wild_list) + strlen(ex_args) + 1);
                strcat(xx, ex_args);
                strcat(xx, " ");
                strcat(xx, wild_list);
                search(xx, args_wild);
                status = execvp(p, args_wild);
              }
              l++;
            }
          }

          else {
            status = execvp(p, args);
          }
          if (status != -1){
            printf("Executing %s\n", args[0]);
          }
          if (status == -1){
            exit(EXIT_FAILURE);
          }
        }
        else if (pid > 0){
          int childStat;
          waitpid(pid, &childStat, 0);
        }
        else{ //error
          perror("Command not found.");
        }
      }
    }    
  } 
}

// exit function
void exitshell(){
  exit(0); //exit
}

// which function
char *which(char *command, struct pathelement *pathlist ){
  while(pathlist->next != NULL){ //loop through paths
    char* loc_com = (char *) malloc(buf);
    strcat(loc_com, pathlist->element);
    strcat(loc_com, "/");
    strcat(loc_com, command);
    if (access(loc_com, F_OK) != -1){
      return loc_com; //return dir found
    }
    else{
      pathlist = pathlist->next;
    }
  }
  if(pathlist->next == NULL){
    char* loc_com = (char *) malloc(buf);
    strcpy(loc_com, pathlist->element);
    strcat(loc_com, "/");
    strcat(loc_com, command);
    if(access(loc_com, F_OK) != -1){
      return loc_com; //return dir found
    }
    else{
      printf("Command not found."); //not found
      return("");
    }
  }
} 

// where function
char *where(char *command, struct pathelement *pathlist ){
  char* total_loc = malloc(sizeof(buf)); //allocate storage
  while (pathlist != NULL){ //loop through paths
    char* loc_com = malloc(sizeof(buf));
    strcpy(loc_com, pathlist->element);
    strcat(loc_com, "/");
    strcat(loc_com, command);
    if (access(loc_com, F_OK) != -1){
      strcat(total_loc, loc_com); //add new found dir to total_loc
      pathlist = pathlist->next;
    }
    else{
      pathlist = pathlist->next;
    }
  }
  if(strcmp(total_loc, "") != 0){
    return(total_loc); //return dir found
  }
  else{
    return("Command not found."); //not found
  }
}

// cd function
void cd(char *pth, char *prev){
  //append our path to current path
  char *path = malloc(sizeof(*path));
  strcpy(path,pth);
  char cwd[buf];
  //get current directory to add args to
  getcwd(cwd,sizeof(cwd));
  if (strcmp(path, "..") == 0){
    chdir(cwd);
    return;
  }
  //create string to check
  strcat(cwd,"/");
  if(strcmp(path, "-")==0){
    chdir(prev);
    return;
  }
  //if directory is passed in as a whole path form
  if(strstr(path,"/")){
    if(chdir(path)!=0){
      chdir(path);
    }
    return;
  }
  //if not passed in whole path form
  strcat(cwd, path);
  int exists = chdir(cwd);
  if(exists != 0){
    perror("cd");
  }
  else{
    chdir(cwd);
  }
}

// pwd function
void printwd(){
  char cwd[buf];
  getcwd(cwd,sizeof(cwd)); //get dir
  printf("%s\n", cwd); //print dir
}

// list function
void list (char *dir){
  DIR* direct;
  struct dirent* entryp;
  direct = opendir(dir);
  if(direct == NULL){
    printf("Can't open directory %s\n", dir); //error opening dir
  }
  else{
    while((entryp = readdir(direct)) != NULL){ //loop through files
      printf("%s\n", entryp->d_name); //print files
    }
    closedir(direct); //close
  }
} 

// pid function
void printpid(){
  printf("Process ID: %d\n", getpid()); //get pid and print it
}

// kill function
void kill_process(char **args, int count){
  if (count == 2){
    kill(atoi(args[1]), SIGKILL); //kill given process
  }
  else if(strstr(args[1], "-") != NULL){
    kill(atoi(args[2]), atoi(++args[1])); 
  }
}

// printenv function if second arg is given (env variable)
void printenvvar(char *env_var){
  char *env = getenv(env_var); //get var from environment
  if(env != NULL){
    printf("%s\n", env); //print var
  }
  else {
    printf("Enviornment does not exist.\n"); //var does not exist
  }
}

// printenv function for one arg (whole env is printed)
void printenv(char **envp){
  char **env;
  env = envp; //get env
  while(*env){
    printf("%s\n", *env); //print env
    env++;
  }
}

// setenv function
void set_env(char* envname, char* envval){
  setenv(envname, envval, 1); //set environment variable to envval
}

// function to remove new line when command is entered.
void newline(char* string){
  int len;
  if((len = strlen(string))>0){
    if(string[len-1] == '\n'){ //if \n exists
      string[len-1] = '\0'; //overwrite \n with \0
    }
  }
}

// function to search cmd for args
int search(char* command, char** parameters){
  int i;
  for(i=0; i<MAX_CANON; i++){
    parameters[i] = strsep(&command, " "); //step through cmd
    if(parameters[i] == NULL){
      return i;
      break;
    }
  }
}

// function to compare chars for wild card feature
int match(char *first, char * second){
    if (*first == '\0' && *second == '\0')
        return 1; //false(first & second do not match)

    if (*first == '*' && *(first+1) != '\0' && *second == '\0')
        return 0; //true (first & second match)
 
    if (*first == '?' || *first == *second)
        return match(first+1, second+1); // recursive
 
    if (*first == '*')
        return match(first+1, second) || match(first, second+1); //recursive
    return 0;
}

//Checks if the string inputed has a wildcard variable.
int wildcardhelper(char *check_string){
  int i = 0;
  const char *invalid_characters = "*?";
  char *c = check_string;
  while (*c){
    if (strchr(invalid_characters, *c)){
          i = 1;
       }
       c++;
  }
  if (i == 1){
    return 1;
  }
  else{
    return 0;
  }
}

// function to free path list when the env is changed
void freepathlist(struct pathelement *head){
  struct pathelement *current = head;
  struct pathelement *temp;
  while(current->next != NULL){ //iterate through linked list a free each item
    temp = current;
    current = current->next;
    free(temp);
  }
  free(current); //free the whole list after all items are freed
}

// Signal Handler for SIGINT
void sigintHandler(int sig_num){
  signal(SIGCHLD, sigintHandler);
  fflush(stdout);
  return;
}

// Signal Handler for SIGSTP
void signalSTPHandler(int sig_num){
  signal(SIGTSTP, signalSTPHandler);
  fflush(stdout);
  return;
}

// wildcard function to list all applicable files
char *list_wildcard (char *dir, char* wildcard){
  int wild_size=0;
  int space_cnt = 0;
  char *wild_list;
  DIR* direct;
  struct dirent* entryp;
  direct = opendir(dir); //open dir
  while((entryp = readdir(direct)) != NULL){
    if (match(wildcard, entryp->d_name) == 1){ //run match
      if (strcmp(entryp->d_name, "..") != 0 && strcmp(entryp->d_name, ".") != 0 && strcmp(entryp->d_name, ".git") != 0){
        wild_size += strlen(entryp->d_name);  //increment the size of the list
        space_cnt++;
        if (opendir(entryp->d_name) != NULL){
          list_wildcard(entryp->d_name, wildcard); //list applicable files
        }
      }
    }
  }
  wild_size += space_cnt +1;
  wild_list = malloc(wild_size);

  DIR* direct1;
  struct dirent* entryp1;
  direct1 = opendir(dir); //open dir
  while((entryp1 = readdir(direct1)) != NULL){
    if (match(wildcard, entryp1->d_name) == 1){ //run match
      if (strcmp(entryp1->d_name, "..") != 0 && strcmp(entryp1->d_name, ".") != 0 && strcmp(entryp1->d_name, ".git") != 0){
        strcat(wild_list, entryp1->d_name); //add to list
        strcat(wild_list, " ");
        if (opendir(entryp1->d_name) != NULL){
          list_wildcard(entryp1->d_name, wildcard); //list applicable files
        }
      }
    }
  }
  return wild_list; //return the list
}

// turn cmd args to string... function used to see if wildcard is called in cmd line
char * args_to_string(char ** args, int argsct){
  char *command = malloc(512);
  int i = 0;
  while(args[i] != NULL){
    strcat(command, args[i]);
    strcat(command, " ");
    i = i + 1;
  }
  return command;
}