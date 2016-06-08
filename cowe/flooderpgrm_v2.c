/* flooderpgrm.c
 *
 * Main program for the Flooder.  Checks the 'listen' directory
 * regularly for either a 'start' or 'stop' file, which is placed
 * in the directory by the flooderlistener.pl socket program.
 * A 'start' file signals the flooder to start the udpinject.sh
 * script, which sends UDP packets to the specified destimation
 * for the specified interval, then pauses for the specified 
 * interval, then resumes sending UDP packets for the specified
 * interval, in a loop.  A 'stop' file signals the flooder to 
 * stop sending UDP packets and end the udpinject.sh script.
 *
 */

#include <fcntl.h>
#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int alarm_fired = 0;
pid_t pid;
const char *ps_argv[] = {"udpinject.sh",0};
const char LOG_FILE[] = "flooder.log";

void ding(int sig)
{
  alarm_fired = 1;
}

//void CreateChild()
int CreateChild()
{
  pid = fork();
  switch(pid)
  {
    case -1:
      /* Failure */
      perror("fork failed");
      exit(1);
    case 0:
      /* Child */
      printf("Creating child process...\n");
      //sleep(5);
      //kill(getppid(), SIGALRM);
      //execv("/home/jim","udpinject.sh","",0);
      //execv("/home/jim",ps_argv);
      printf("Starting udpinject script...\n");
      // Open log file
      //FILE *logfile

      int out = open(LOG_FILE, O_RDWR|O_CREAT|O_APPEND,0600);
      if (-1 == out) { perror("opening output.log"); return 255; }

      int err = open("cerr.log", O_RDWR|O_CREAT|O_APPEND, 0600);
      if (-1 == err) { perror("opening cerr.log"); return 255; }

      int save_out = dup(fileno(stdout));
      int save_err = dup(fileno(stderr));

      if (-1 == dup2(out, fileno(stdout))) { perror("cannot redirect stdout"); return 255; }
      if (-1 == dup2(err, fileno(stderr))) { perror("cannot redirect stderr"); return 255; }

      /*
      logfile = fopen(LOG_FILE, "w");
      char output[1000];
      if( logfile == NULL )
      {
        printf("Cannot open file %s\n", LOG_FILE);
        exit(8);
      }
      */

      system("/home/jim/udpinject.sh");
      //fwrite(output, sizeof(char), 1000, logfile);

      fflush(stdout); close(out);
      fflush(stderr); close(err);

      dup2(save_out, fileno(stdout));
      dup2(save_err, fileno(stderr));

      close(save_out);
      close(save_err);

      exit(0);
  }
}

int main()
{
  int createChild = 1;

  printf("Flooder program starting\n");

  /* if we get here we are the parent process */
  while(1)
  {
    printf("Waiting for file...\n");
    //(void) signal(SIGALRM, ding);
    sleep(2);
    /* If a start file is found, create child */
    if(createChild)
    {
      CreateChild();
      createChild = 0;
    }
    
    sleep(5);
    if(createChild == 0) { createChild = 1; }
  }

  pause();
  if (alarm_fired) printf("Ding!\n");

  printf("done\n");
  exit(0);
}

