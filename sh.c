//Anthony Sette and Matthew Grossman
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
#include <sys/time.h>
#include <signal.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <glob.h>
#include "sh.h"
#include <stdbool.h>
#include <utmpx.h>
#include "wm_list.h"
#include "wu_list.h"


#define buf 1024 //buf size
#define max 2048 //max size

struct watchuser_list *uh = NULL; // watchuser linked list head
struct watchuser_list *ut = NULL; // watchuser linked list tail
struct watchmail_list *mh = NULL; // watchmail linked list head
struct watchmail_list *mt = NULL; // watchmail linked list tail
pthread_mutex_t user_lock;

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
  bool firstTime = true; // Indicates if this is the first time that watchuser is being run

  struct watchuser_list *uh; // watchuser linked list head
  struct watchuser_list *ut; // watchuser linked list tail
  struct watchmail_list *mh; // watchmail linked list head
  struct watchmail_list *mt; // watchmail linked list tail

  // get user and pass then start in home dir
  uid = getuid(); //get uid
  password_entry = getpwuid(uid); //get pass
  homedir = password_entry->pw_dir; //start in home dir

  if ( (pwd = getcwd(NULL, PATH_MAX+1)) == NULL )
  {
    perror("getcwd");
    exit(2);
  }
  struct sigaction si;
  si.sa_handler = &sig_child_handler;
  sigemptyset(&si.sa_mask);
  si.sa_flags = SA_RESTART | SA_NOCLDSTOP;
  if(sigaction(SIGCHLD, &si, 0) == -1){
    perror(0);
    exit(1);
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
    //watchuser
    else if(strcmp(args[0], "watchuser") == 0){ // this should handle adding and removing the users, watchuser ITSELF should just track whether somebody has logged on or not 
      printf("Executing built-in %s\n", args[0]);
      pthread_t wat_thread;
      if(argsct > 3){ // we can only have as many as three arguments. If this is run with no arguments, then we just print the list starting at the most recent user
        fprintf(stderr, "Invalid argument count for function %s\n", args[0]);
      }
      else if(argsct == 2){
        if(firstTime){ // means this is the first time watchuser is being run in this program
          pthread_create(&wat_thread, NULL, watchuser, "User Thread");
          !firstTime;
        }
        else{ // then we just want to add a user to the linked list
          pthread_mutex_lock(&user_lock);
          struct watchuser_list *user = malloc(sizeof(struct watchuser_list)); // since the node is a pointer, we need space for it
          user->node = malloc(1024); // set the amount of space the user value has to be whatever the size of our username is
          strcpy(user->node, args[1]); // set the value of the node in the user to be the username that is passed into the command line
          if(uh == NULL){ // If there is no head, that means there is nothing in the linked list yet
            uh = user;
            ut = user; // also, if this case is fulfilled, since there is only one node, that node is both the head, and the tail
          }
          else{ // means there is at least one node in the list, and we can add this next node as normal, meaning it goes to the end of the list
            ut->next = user; // set the new end of the list (this being the node after the tail) to be the new user node
            user->prev = ut; // sets the tail to now be the node BEFORE THE TAIL
            ut = user; // has the tail pointer now point to the new user node that was just created
          }
          pthread_mutex_unlock(&user_lock); // now that we are done editing the linked list, we can get rid of our lock!
        }
      } // means that we just have the function call and then the username we want to track
      else if(argsct == 3){ // all this means is that we have the second optional argument, that being "off", meaning we are removing a user
        if(strcmp(args[2], "off") == 0){ // this means WE WANT TO GET RID OF A USER
          pthread_mutex_lock(&user_lock); // lock the process so nothing gets out by accident
          struct watchuser_list *t; // Creating this will allow us to do our moving around later on!
          // we will want to first check and see if we are either removing the head or the tail, and then if we are removing the head, if this is the last node in the entire list
          t = uh; // Since we have to go through the list to check which node we are removing, might as well start from the top, and search through to the end
          bool userThere = true; // this is for our while loop. Since a user can only be added to the watch list once, when we remove that user, we don't have to go through any further than that, so we can stop searching once the user is removed
          while(userThere){ // Until we delete the specified user. We will also add a case in which we get to the end of the list and the user turns out to not be in the list, though if the program is used correctly we won't reach that condition
            if(strcmp(t->node, args[1]) == 0){ // means the node that we are currently on is the one we want to remove
             if(strcmp(t->node, uh->node) == 0){
              if(t->next != NULL){
                uh = uh->next; // sets the head to be the node AFTER the OLD HEAD, that way we can just remove the temp node and be done with it!
              } // means the head is NOT the only node left in the list
              else{
                uh = NULL;
                ut = NULL;
              } // means that the head is the final node in the list. Since the temp pointer is already pointing to the header node, we can set the header and tail pointers to NULL, and then just free the temp pointer
            }
            else if(strcmp(t->node, ut->node) == 0){ // means that the tail is the node we want to remove, so we need to move the tail pointer so that we can remove the temp node without causing any issues
              ut = ut->prev; // sets the new tail pointer to be to the node BEFORE the OLD TAIL
            }
            else{ // means we aren't in the beginning of the list, or the end. Just somewhere in the middle
              t->prev->next = t->next; // sets it so the "next" pointer of the node preceding the node about to be deleted is now set to the node AFTER the temp node
              t->next->prev = t->prev; // sets it so the "prev" pointer of the node succeeding the node about to be deleted is now set to the node BEFORE the temp node
            }
            free(t->node);
            free(t);
            userThere = false; // Get rid of the temp node, and change our "user still there" boolean to reflect that the user is now not on the list anymore
          }
          else{ // means that the current node we are on is NOT the node that we want to delete, so we just move on to the next node
            if(t->next == NULL){ // if we have reached the end of the list, just print that the user isn't in the list and move on
              printf("User is not in the Watch List\n");
              userThere = false;
            }
            else{ // means we aren't at the end of the list, so we just move on to the next node
              t = t->next;
            }
          }
        }
        pthread_mutex_unlock(&user_lock);
        }
        else{
          fprintf(stderr, "Included three arguments without removing a user \n");
        }
      }
      else{
        struct watchuser_list *curr = ut;
        while(ut != NULL){
          printf("%s\n", curr->node);
          curr = curr->prev;
        }
      }
    }
    //watchmail
    else if(strcmp(args[0], "watchmail") == 0){
      printf("Executing built-in %s\n", args[0]);
      pthread_t track;
      if(argsct == 1 || argsct > 3){
        fprintf(stderr, "Invalid argument count\n");
      }
      else if(argsct == 2){ // means we have the file name. Need to make sure it exists, and then we can add it
        if(access(args[1], F_OK) == -1){ // if the file doesn't exist
          fprintf(stderr, "File does not exist\n");
        }
        else{
          struct watchmail_list *mail = malloc(sizeof(struct watchmail_list));
          mail->node = malloc(1024);
          strcpy(mail->node, args[1]);
          if(mh == NULL){ // if we are adding the first node
            mh = mail;
            mail->next = NULL;
          }
          else{
            mt->next = mail;
            mail->prev = mt;
            mt = mail;
          }
          pthread_create(&track, NULL, watchmail, args[1]); // create the thread to track the file
        }
      }
        else if(argsct == 3){ // means we are going to stop tracking mails for a file
          if(strcmp(args[2], "off") == 0){
            struct watchmail_list *tmp;
            bool stillThere = true; // checking if the mail has been deleted yet
            tmp = mh;
           // pthread_cancel(&track);
            while(stillThere){
              if(strcmp(args[1], tmp->node) == 0){ // means this is the node we want to get rid of
                if(tmp == mh){
                  if(tmp->next == NULL){
                    mh = NULL;
                    mt = NULL;
                  }
                  else{ // means the head isn't the last node left
                    mh = mh->next;
                  }
                }
                else if(tmp == mt){ // if the node is the tail
                  mt = mt->prev;
                  mt->next == NULL;
                }
                else{ // means the node is in the middle
                  tmp->next->prev = tmp->prev;
                  tmp->prev->next = tmp->next;
                }
                free(tmp->node);
                free(tmp);
                stillThere = false;
              }
              else{ // means this isn't the node we are looking for, and we move on
                if(tmp->next == NULL && stillThere){
                  printf("Node not found in list\n");
                  stillThere = false;
                }
                else{
                  tmp = tmp->next;
                }
              }
            }
          }
          else{
            fprintf(stderr, "If inputting two arguments to function %s, can only have 'off' as the last argument", args[0]);
          }
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
      else if (p != ""){ // means that the "which" command found the process and we can fork it 
        pid = fork(); //fork it
        bool isBack = false; // our verification that we are either running a process in the foreground or background 
        if(strcmp(args[argc - 1], "&") == 0){
          isBack = true; // sets our background flag to true
          args[argc-1] = NULL; // and then gets rid of the ampersand, that way we can just pass the argument into the background process... thing...
          argc--;
        } // means that we have a & at the end of the commandline, so this process is going to be run in the background
        if (pid == 0){ // means that we are in the child process
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
          if(isBack){ // means that this process is going to be run in the background
            fclose(stdin);
            open("/dev/null", O_RDONLY);
            execvp(*args, args);
            fprintf(stderr, "unknown command: %s\n", args[0]);
            exit(1);
          }
          else {
            status = execvp(p, args); // run the external command with the arguments
          }
          if (status != -1){ // as long as there is no error with executing the command
            printf("Executing %s\n", args[0]);
          }
          if (status == -1){ // means there was an error in executing the command
            exit(EXIT_FAILURE);
          }
        }
        else if (pid > 0){ // means that this is the parent process
          if(isBack){
            printf("starting background job %d\n", pid);
          }
          else{
            waitpid(pid, &status, 0);
          }
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
void watchuser(char ** args){
  struct utmpx *up;
  while(1){ // run infinitely
    sleep(20);
    setutxent();
    while(up = getutxent()){
      if(up->ut_type == USER_PROCESS){ // means this is the process we want to look in
        pthread_mutex_lock(&user_lock);
        struct watchuser_list *t = uh;
        while(t != NULL){
          if(strcmp(t->node, up->ut_user) == 0){ // means this is the user we want the log-on info about 
            printf("%s has logged on %s from %s\n", up->ut_user, up->ut_line, up->ut_host);
          }
          t = t->next;
        }
        pthread_mutex_unlock(&user_lock);
      }
    }
  }
}
void watchmail(char *name){
  const char *fn = name; // this just saves the parameter to a value
  struct timeval curr; // The struct that will hold the current time and be passed into ctime()
  int prev_size = 0; // The original, and then most recent size of the file (before the file is checked)
  struct stat fil;
  int s;
  char tims[1024];
  while(1){
    sleep(1);
    struct watchmail_list *n = mh; // we need to search through our list until we find the file we want to see if it was updated
    bool notFound = true; // tracks whether or not we've found the file we are looking for. This will handle the logic for printing everything too
    while(notFound){
      if(strcmp(n->node, name) == 0){
        s = stat(fn, &fil);
        if(prev_size == 0){ // means we don't have the original size of this file
          prev_size = fil.st_size;
        } // now that we have the original size, we can check to see if the file size is bigger than it once was, and if it was, then it was changed!!
        if(fil.st_size > prev_size){ // means the file is different!
          gettimeofday(&curr, NULL); // the value is saved into "curr"
          strcpy(tims,ctime(curr.tv_sec));
          printf("BEEP\a You've got Mail in %s at %s\n", fn, tims);
          prev_size = fil.st_size;
          notFound = false;
        }
        else{
          printf("File %s is unchanged\n", fn);
          notFound = false;
        }
      }
      else{
        if(n->next == NULL && notFound){
          printf("File not found\n");
          notFound = false; // basically means we got to the end of our list and didn't find the file
          pthread_exit(&exit);
        }
        else{
          n = n->next;
        }
      }
    }
  }
}
void sig_child_handler(int sig){
  int s_err = errno; // This is the error associated with the signal
  while(waitpid((pid_t)(-1), 0, WNOHANG) > 0){
    errno = s_err;
  }
}