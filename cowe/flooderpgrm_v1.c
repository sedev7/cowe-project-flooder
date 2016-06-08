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

#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int alarm_fired = 0;

void ding(int sig)
{
  alarm_fired = 1;
}

int main()
{
  pid_t pid;

  printf("Flooder program starting\n");

  pid = fork();
  switch(pid)
  {
    case -1:
      /* Failure */
      perror("fork failed");
      exit(1);
    case 0:
      /* Child */
      sleep(5);
      kill(getppid(), SIGALRM);
      exit(0);
  }

  /* if we get here we are the parent process */
  printf("waiting for alarm to go off\n");
  (void) signal(SIGALRM, ding);

  pause();
  if (alarm_fired) printf("Ding!\n");

  printf("done\n");
  exit(0);
}

