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
 * v4:
 *  - Clean out old code.
 *  - Add code to parse start and stop file.
 *  - Add code to start udpinject.sh with start file parameters.
 *
 */

#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>


/*
 *
 * Global Variables
 *
 */

const char *ps_argv[] = {"udpinject.sh",0};
//const char LOG_FILE[] = "flooder.log";
const char LOG_FILE[] = "/home/jim/cowe/flooder.log";
const char ERROR_LOG_FILE[] = "/home/jim/cowe/error.log";
//const char START_FILE[] = "listen/start";
const char START_FILE[] = "start";
//const char STOP_FILE[] = "listen/stop";
const char STOP_FILE[] = "stop";
char *LISTEN_DIR = "/home/jim/cowe/listen";
char *UNKNOWN_FILE;
//int status;
pid_t pid;


/*
 *
 * Functions
 *
 */

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

int GetFile()
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
      {
        //printf("...found 'start' file...\n");
        return 1;
      }
      else if (strcmp("stop", dir->d_name) == 0)
      {
        //printf("...found 'stop' file...\n");
        return 2;
      }
      else
      {
        //printf("...found unknown file...\n");
        // Get the name of the file so we can delete it
        UNKNOWN_FILE = dir->d_name; 
        return 3;
      }
    }
  }
  return 0;
}

int CreateChild()
{
  int status;
  int cstatus;
  int pstatus;

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

      // Get the child pid
      printf("In [CreateChild] - getting child pid with getpid(): %d\n", getpid());
    
      printf("In [CreateChild] - starting udpinject script...\n");

      // Redirect stdout and stderr to log file
      int out = open(LOG_FILE, O_RDWR|O_CREAT|O_APPEND,0600);
      if (-1 == out) { perror("opening flooder.log"); return 255; }

      //int err = open("error.log", O_RDWR|O_CREAT|O_APPEND, 0600);
      int err = open(ERROR_LOG_FILE, O_RDWR|O_CREAT|O_APPEND, 0600);
      if (-1 == err) { perror("opening error.log"); return 255; }

      int save_out = dup(fileno(stdout));
      int save_err = dup(fileno(stderr));

      if (-1 == dup2(out, fileno(stdout))) { perror("cannot redirect stdout"); return 255; }
      if (-1 == dup2(err, fileno(stderr))) { perror("cannot redirect stderr"); return 255; }

      // Start the udpinject script
      //system("/home/jim/udpinject.sh");
      //execl("/home/jim/udpinject.sh", "udpinject.sh", (char *) NULL);
      execlp("/home/jim/udpinject.sh", "udpinject.sh", (char *) NULL);

      //system("packit -m inject -t UDP -s 10.10.10.208 -d 10.10.10.9 -S 403 -D 80 -c 300 -b 20 -p '0x 68 76 6D 72 20 74 65 73 74 20 75 70 64 20 70 61 63 6B 65 74 73'");

      fflush(stdout); close(out);
      fflush(stderr); close(err);

      dup2(save_out, fileno(stdout));
      dup2(save_err, fileno(stderr));

      close(save_out);
      close(save_err);

      //exit(0);
      //break;
  //}
      
      //waitpid(pid, &status, 0);
      waitpid(pid, &status, WNOHANG);
      //int childPid = wait(&cstatus);
      //printf("A child with pid %d terminated with exit code %d\n", childPid, cstatus>>8);
      //waitpid(getpid(), &cstatus, WNOHANG);
      //waitpid(pid, &cstatus, WNOHANG);
      //waitpid(getppid(), &pstatus, WNOHANG);
      //waitpid(getpid(), &cstatus, WNOHANG);
      printf("In [CreateChild] - child pid: %d\n", getpid());
      printf("In [CreateChild] - child waitpid status: %d\n", cstatus);
      printf("In [CreateChild] - parent pid: %d\n", getppid());
      printf("In [CreateChild] - parent pid waitpid status: %d\n", pstatus);

      //while (wait(&cstatus) != pid)
      //;

      do
      {
        // Get pid exit status
        //pid_t w = waitpid(getpid(), &cstatus, WUNTRACED | WCONTINUED);
        //pid_t w = waitpid(pid, &cstatus, WUNTRACED | WCONTINUED);
        //pid_t w = waitpid(-1, &cstatus, WUNTRACED | WCONTINUED);
        pid_t w = waitpid(getppid(), &cstatus, WUNTRACED | WCONTINUED);
        if(w == -1) { perror("waitpid"); exit(EXIT_FAILURE); }

        if (WIFEXITED(cstatus))
        {
          printf("exited, status=%d\n", WEXITSTATUS(cstatus));
        }
        else if (WIFSIGNALED(cstatus))
        {
          printf("killed by signal %d\n", WTERMSIG(cstatus));
        }
        else if (WIFSTOPPED(cstatus))
        {
          printf("stopped signal %d\n", WSTOPSIG(cstatus));
        }
        else if (WIFCONTINUED(cstatus))
        {
          printf("continued\n");
        }
      } 
      while (!WIFEXITED(cstatus) && !WIFSIGNALED(cstatus));

      /* 
      // If child pid exited, kill any zombies
      if (cstatus < 0)
      {
        KillChildPid(getppid());
        KillChildPid(getpid());
      }
      */ 

      exit(EXIT_SUCCESS);
       
      /* 
      EC_CLEANUP_BGN
        EC_FLUSH("CreateChild");
      EC_CLEANUP_END
      */ 

      exit(0);
      //_Exit(0);
      //return 0;
  }
}

int CreateChild1()
{
      printf("In [CreateChild1] - starting udpinject script...\n");

      // Redirect stdout and stderr to log file
      int out = open(LOG_FILE, O_RDWR|O_CREAT|O_APPEND,0600);
      if (-1 == out) { perror("opening flooder.log"); return 255; }

      //int err = open("error.log", O_RDWR|O_CREAT|O_APPEND, 0600);
      int err = open(ERROR_LOG_FILE, O_RDWR|O_CREAT|O_APPEND, 0600);
      if (-1 == err) { perror("opening error.log"); return 255; }

      int save_out = dup(fileno(stdout));
      int save_err = dup(fileno(stderr));

      if (-1 == dup2(out, fileno(stdout))) { perror("cannot redirect stdout"); return 255; }
      if (-1 == dup2(err, fileno(stderr))) { perror("cannot redirect stderr"); return 255; }

      // Start the udpinject script
      system("/home/jim/udpinject.sh");
      //execl("/home/jim/udpinject.sh", "udpinject.sh",  NULL);

      //system("packit -m inject -t UDP -s 10.10.10.208 -d 10.10.10.9 -S 403 -D 80 -c 300 -b 20 -p '0x 68 76 6D 72 20 74 65 73 74 20 75 70 64 20 70 61 63 6B 65 74 73'");

      fflush(stdout); close(out);
      fflush(stderr); close(err);

      dup2(save_out, fileno(stdout));
      dup2(save_err, fileno(stderr));

      close(save_out);
      close(save_err);

      printf("In [CreateChild1] - udpinject script finished, exiting...\n");

      exit(0);
}

int KillChildPid(pid_t cpid)
{
    int status;

    if (cpid != 0)
    {
      printf("Sending SIGHUP signal to pid %d\n", getpid());
      kill(pid, SIGHUP);

      // Wait for pid so we don't create a zombie
      waitpid(pid, &status, 0);

      // Success
      return 0;
    }

    // Something went wrong
    return 1;
}


/*
 *
 * Main Program
 *
 */

int main()
{
  //int createChild = 1;
  char cwd[1024];
  int status;

  printf("Flooder program starting\n");

  while(1)
  {
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
      // Change to the listen directory
      if (strcmp(getcwd(cwd, sizeof(cwd)), LISTEN_DIR) != 0)
      {
        if (chdir(LISTEN_DIR) != 0)
          perror("Error changing to listening directory\n");
      }

      switch(GetFile())
      {
        case 1:
          printf("...found 'start' file...\n");
          //CreateChild();
          // Delete the file 
          if (remove(START_FILE) != 0)
            perror("Error deleting 'start' file\n");
          CreateChild();
          //CreateChild1();
          //waitpid(getpid(), &status, 0);
          //waitpid(getppid(), &status, 0);
          waitpid(-1, &status, WNOHANG);
          //KillChildPid(getpid());
          break;

        case 2:
          printf("...found 'stop' file...\n");
          printf("Killing child pid %d\n", getpid());
          if (KillChildPid(getpid()) == 0)
            printf("Child pid successfully killed\n");
          else
            printf("Error killing child pid\n");
          // Delete the file 
          if (remove(STOP_FILE) != 0)
            perror("Error deleting 'stop' file\n");
          break;

        default:
          printf("...found unknown file...\n");
          // Delete the file 
          if (remove(UNKNOWN_FILE) != 0)
            perror("Error deleting unknown file\n");
          break;
      }
    }

    sleep(5); 

    // Check for running pids (no zombies)
    //waitpid(getpid(), (int *) 0, WNOHANG);
    //waitpid(getpid(), &status, WNOHANG);
    //printf("In [while] - waitpid status: %d\n", status);

  }

  //pause();

  printf("done\n");
  exit(0);
}

