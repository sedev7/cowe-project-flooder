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

#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static int alarm_fired = 0;
pid_t pid;	/* Parent pid */
//pid_t cpid;	/* Child pid */
const char *ps_argv[] = {"udpinject.sh",0};
const char LOG_FILE[] = "flooder.log";
char *LISTEN_DIR = "listen";
int status;

void ding(int sig)
{
  alarm_fired = 1;
}

int IsDirectoryEmpty(char *dirname)
{
  int n = 0;
  struct dirent *d;
  DIR *dir = opendir(dirname);
  if (dir == NULL) // Not a directory or doesn't exist
    return 1;
  while (( d = readdir(dir)) != NULL )
  {
    if(++n > 2)
      break;
  }
  closedir(dir);
  if (n <= 2) // Directory is empty (only '.' and '..' found)
    return 1;
  else
    return 0;
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
      printf("Creating child process...pid: %d\n", pid);
      //sleep(5);
      //kill(getppid(), SIGALRM);

      // Get the child pid
      printf("In [CreateChild] - getting pid with getpid(): %d\n", getpid());
    
      //cpid = pid;
      //cpid = getpid();
      //printf("In [CreateChild] - checking value of cpid: %d\n", cpid);

      printf("In [CreateChild] - starting udpinject script...\n");

      // Redirect stdout and stderr to log file
      int out = open(LOG_FILE, O_RDWR|O_CREAT|O_APPEND,0600);
      if (-1 == out) { perror("opening output.log"); return 255; }

      int err = open("cerr.log", O_RDWR|O_CREAT|O_APPEND, 0600);
      if (-1 == err) { perror("opening cerr.log"); return 255; }

      int save_out = dup(fileno(stdout));
      int save_err = dup(fileno(stderr));

      if (-1 == dup2(out, fileno(stdout))) { perror("cannot redirect stdout"); return 255; }
      if (-1 == dup2(err, fileno(stderr))) { perror("cannot redirect stderr"); return 255; }

      // Start the udpinject script
      system("/home/jim/udpinject.sh");

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
    //printf("Waiting for file...\n");
    //(void) signal(SIGALRM, ding);
    //sleep(2);
    /* If a start file is found, create child */
    if(createChild)
    {
      CreateChild();
      createChild = 0;
    }
    
    sleep(5);
    //printf("Killing child pid %d\n", cpid);
    printf("Killing child pid %d\n", getpid());
    if (getpid() != 0)
    {
      printf("Sending SIGHUP signal to pid %d\n", getpid());
      //kill(pid, SIGKILL);
      kill(pid, SIGHUP);
      //kill(cpid, SIGHUP);
      waitpid(pid, &status, 0);
    }
    //sleep(5);

    if(createChild == 0) { createChild = 1; }

    // Check the listen directory for a file
    printf("Checking for files...\n");
    int isEmpty =  IsDirectoryEmpty(LISTEN_DIR);
    if (isEmpty)
      printf("Directory 'listen' is empty\n");
    else
      printf("Directory 'listen' is not empty\n");

    // Process any files found in 'listen' directory
    if (!isEmpty)
    {
      DIR *d;
      struct dirent *dir;
      d = opendir(LISTEN_DIR);
      if (d)
      {
        while ((dir = readdir(d)) != NULL)
        {
          if (strcmp(".", dir->d_name) == 0 || strcmp("..", dir->d_name) == 0)
            continue;
          else if (strcmp("start", dir->d_name) == 0)
            printf("...found 'start' file...\n");
          else if (strcmp("stop", dir->d_name) == 0)
            printf("...found 'stop' file...\n");
          else
            printf("...found unknown file...\n");
        }
      }
       
    }
    sleep(5); 
  }

  pause();
  if (alarm_fired) printf("Ding!\n");

  printf("done\n");
  exit(0);
}

