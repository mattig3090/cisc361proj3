//Anthony Sette and Matthew Grossman
#include "sh.h"
#include <signal.h>
#include <stdio.h>

void sig_handler(int signal); 

int main( int argc, char **argv, char **envp )
{
  /* put signal set up stuff here */
  signal(SIGSTP, sig_handler);

  return sh(argc, argv, envp);
}

void sig_handler(int signal)
{
  if(sig == SIGSTP){
    fflush(stdout);
  }
  /* define your signal handler */
}

