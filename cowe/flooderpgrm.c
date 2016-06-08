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
 * v7:
 *  - Clean out old code.
 *  - Add code to clean out the flooder.log and error.log file
 *    at startup.
 *
 * v6:
 *  - Clean out old code.
 *  - Add timer to restart packet injection at the end of time the
 *    specified time interval (on, off, on, off, ... sequence).
 *
 * v5:
 *  - Clean out old code.
 *  - Note: a zombie process is created by CreateChild() method
 *          when udpinject.sh script ends normally, but the zombie
 *          is destroyed the next time the method is called, so 
 *          there is only ever one in the process table.  It is
 *          also destroyed if a 'stop' file is sent.
 *  - Add a struct for flooder data for the 'start' file.
 *
 * v4:
 *  - Clean out old code.
 *  - Add code to parse start and stop file.
 *  - Add code to start udpinject.sh with start file parameters.
 *
 */

#include <dirent.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "ec.h"


/*
 *
 * Macro Declarations
 *
 */

#ifndef _EC_H_
  #define _EC_H_
#endif
#ifndef ec_in_cleanup
  #define ec_in_cleanup
#endif
#define ec_neg1(x) ec_cmp(x, -1)
#define ec_cmp

/*
typedef int bool;
#define true 1
#define false 0
*/


/*
 *
 * Global Variables
 *
 */

const char *ps_argv[] = {"udpinject.sh",0};
const char LOG_FILE[] = "/home/jim/cowe/flooder.log";
const char ERROR_LOG_FILE[] = "/home/jim/cowe/error.log";
const char START_FILE[] = "start";
const char STOP_FILE[] = "stop";
char *LISTEN_DIR = "/home/jim/cowe/listen";
char *FILE_DIR = "/home/jim/cowe";
char *UNKNOWN_FILE;
//int IsFlooding = false;
int IsFlooding = 0;
//int IsRunning = false;
int IsRunning = 0;
int UseFileDir = 0;
pid_t current_pid;

/*
typedef struct
{
  char * sipaddr;	// Source IP address (this flooder instance)	
  char * dipaddr;	// Destination IP address (where packets will be sent)
  char *  sportno;	// Source port number (this flooder instance)
  char *  dportno;	// Destination port number (where packets will be sent)
  char * tinterval;	// Time interval for running
} *flooder;
*/

typedef struct
{
  char sipaddr[16];	// Source IP address (this flooder instance)	
  char dipaddr[16];	// Destination IP address (where packets will be sent)
  char sportno[6];	// Source port number (this flooder instance)
  char dportno[6];	// Destination port number (where packets will be sent)
  char tinterval[6];	// Time interval for running
} flooder;

flooder FLOODER;


/*
 *
 * Function Declarations
 *
 */

void Handler(int signum);


/*
 *
 * Functions
 *
 */

void InitializeFlooderObject()
{
  int i;

  for (i=0; i<= sizeof(FLOODER.sipaddr); i++)
  {
    FLOODER.sipaddr[i] = '\0';
  }

  for (i=0; i<= sizeof(FLOODER.dipaddr); i++)
  {
    FLOODER.dipaddr[i] = '\0';
  }

  for (i=0; i<= sizeof(FLOODER.sportno); i++)
  {
    FLOODER.sportno[i] = '\0';
  }

  for (i=0; i<= sizeof(FLOODER.dportno); i++)
  {
    FLOODER.dportno[i] = '\0';
  }

  for (i=0; i<= sizeof(FLOODER.tinterval); i++)
  {
    FLOODER.tinterval[i] = '\0';
  }
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

//flooder ParseStartFile() 
void ParseStartFile() 
{
  char cwd[1024];
  char fdata[41];
  flooder _Flooder;

  InitializeFlooderObject();

  /*
  if (UseFileDir)
  {
    // Change to the 'start' file directory
    if (strcmp(getcwd(cwd, sizeof(cwd)), FILE_DIR) != 0)
    {
      if (chdir(FILE_DIR) != 0)
        perror("In [ParseStartFile] - error changing to 'start' file directory\n");
    }
  }
  else
  */
  {
    // Change to the listen directory
    if (strcmp(getcwd(cwd, sizeof(cwd)), LISTEN_DIR) != 0)
    {
      if (chdir(LISTEN_DIR) != 0)
        perror("In [ParseStartFile] - error changing to listening directory\n");
    }
  }

  // Open the 'start' file and retrieve its contents
  FILE *infile;
  infile = fopen (START_FILE, "r");
  if (infile == NULL)
  {
    printf("In [ParseStartFile] - cannot open file '%s'\n", START_FILE);
    exit(8);
  }

  // Read the file data into a buffer
  // - Source IP address (max size: 15)
  // - Destination IP Address (max size: 15)
  // - Source Port Number (max size: 5)
  // - Destination Port Number (max size: 5)
  // - Time interval (max size: 6)
  //   Field separator ("|")
  //   NULL character ("\0")
  fread(fdata, sizeof(char), 52, infile);

  fclose(infile);

  // Populate the flooder struct with the file data
  // Use a tokenizer to parse the file

  /*
  char *p;
  p = strtok(fdata, "|");
  _Flooder.sipaddr = p;
  p = strtok('\0', "|");       
  _Flooder.dipaddr = p;
  p = strtok('\0', "|");       
  _Flooder.sportno = p;
  p = strtok('\0', "|");       
  _Flooder.dportno = p;
  p = strtok('\0', "|");       
  _Flooder.tinterval = p;
  */

  char *p;
  p = strtok(fdata, "|");
  strcpy(FLOODER.sipaddr, p);
  p = strtok('\0', "|");       
  strcpy(FLOODER.dipaddr, p);
  p = strtok('\0', "|");       
  strcpy(FLOODER.sportno, p);
  p = strtok('\0', "|");       
  strcpy(FLOODER.dportno, p);
  p = strtok('\0', "|");       
  strcpy(FLOODER.tinterval, p);

  _Flooder = FLOODER;

  // Verify we have read the data correctly
  DisplayFlooder(_Flooder);
}

//int CreateChild(flooder f)
int CreateChild()
{
  int status;
  int cstatus;
  int pstatus;
  flooder f = FLOODER;
  pid_t pid;

  printf("In [CreateChild] - verify flooder object...\n");
  DisplayFlooder(f);

  pid = fork();
  switch(pid)
  {
    case -1:
      /* Failure */
      perror("fork failed");
      printf("In [CreateChild] - fork failed - exiting with error\n");
      exit(1);
    case 0:
      /* Child */
      printf("Creating child process...pid: %d\n", pid);

      // Get the child pid
      current_pid = getpid();
      printf("In [CreateChild] - getting child pid with getpid(): %d\n", current_pid);
      printf("In [CreateChild] - assigning child pid to [current_pid]: %d\n", current_pid);
    
      printf("In [CreateChild] - starting packet injection ...\n");

      // Redirect stdout and stderr to log file
      int out = open(LOG_FILE, O_RDWR|O_CREAT|O_APPEND,0600);
      if (-1 == out) { perror("opening flooder.log"); return 255; }

      int err = open(ERROR_LOG_FILE, O_RDWR|O_CREAT|O_APPEND, 0600);
      if (-1 == err) { perror("opening error.log"); return 255; }

      int save_out = dup(fileno(stdout));
      int save_err = dup(fileno(stderr));

      if (-1 == dup2(out, fileno(stdout))) { perror("cannot redirect stdout"); return 255; }
      if (-1 == dup2(err, fileno(stderr))) { perror("cannot redirect stderr"); return 255; }

      //execlp("/home/jim/udpinject.sh", "udpinject.sh", (char *) NULL);

      // Start injecting packets
      execlp("/usr/sbin/packit", "packit", "-m", "inject", "-t", "UDP", "-s", f.sipaddr, "-d", f.dipaddr, "-S", f.sportno , "-D", f.dportno, "-c", "0", "-b", "0", "-p", "'0x 68 76 6D 72 20 74 65 73 74 20 75 70 64 20 70 61 63 6B 65 74 73'", (char *) NULL);

      fflush(stdout); close(out);
      fflush(stderr); close(err);

      dup2(save_out, fileno(stdout));
      dup2(save_err, fileno(stderr));

      close(save_out);
      close(save_err);

      IsFlooding = 1;
      printf("In [CreateChild] - flooder is%sflooding\n", IsFlooding == 1 ? "" : "not ");

      waitpid(pid, &status, WNOHANG);
      printf("In [CreateChild] - child pid: %d\n", getpid());
      printf("In [CreateChild] - child waitpid status: %d\n", status);
  }
}

DisplayFlooder(flooder f)
{
  flooder _Flooder = f;

  // Verify we have read the data correctly
  printf("In [DisplayFlooder] - show flooder:\n");
  printf("   _Flooder.sipaddr:\t %s\n", _Flooder.sipaddr);
  printf("   _Flooder.dipaddr:\t %s\n", _Flooder.dipaddr);
  printf("   _Flooder.dportno:\t %s\n", _Flooder.dportno);
  printf("   _Flooder.sportno:\t %s\n", _Flooder.sportno);
  printf("   _Flooder.tinterval:\t %s\n", _Flooder.tinterval);
}

int KillChildPid(pid_t cpid)
{
    int status;

    if (cpid != 0)
    {
      //printf("Sending SIGHUP signal to pid %d\n", getpid());
      printf("In [KilChildPid] - sending SIGHUP signal to pid %d\n", cpid);
      kill(cpid, SIGHUP);

      // Wait for pid so we don't create a zombie
      waitpid(cpid, &status, 0);

      // Success
      return 0;
    }

    // Something went wrong
    return 1;
}

int KillCurrentPid()
{
    int status;
    int result;

    printf("In [KillCurrentPid] - verify [current_pid]: %d\n", current_pid);

    if (current_pid != 0)
    {
      printf("In [KillCurrentPid] - sending SIGHUP signal to pid %d\n", current_pid);
      result = kill(current_pid, SIGHUP);

      // Wait for pid so we don't create a zombie
      waitpid(current_pid, &status, 0);

      // Success
      printf("In [KillCurrentPid] - [current_pid] successfully killed\n");
      return 0;
    }

    // Something went wrong
    printf("In [KillCurrentPid] - error killing [current_pid] - result: %d\n", result);
    return 1;
}

void Timer(int interval_time, int timer_time, flooder f)
{
  struct sigaction act;		/* Implements action to be taken when signal is delivered */
  struct itimerval itv;		/* Time interval structure */
  flooder FLOODER = f;
  
  memset(&act, 0, sizeof(act));
  act.sa_handler = Handler;				/* Event handler when timer fires */
  act.sa_flags = SA_RESTART;    			/* Don't interrupt system calls */
  ec_neg1( sigaction (SIGALRM, &act, NULL) );		/* Define the type of signal */
  memset(&itv, 0, sizeof(itv));
  itv.it_interval.tv_sec = interval_time;		/* Set the timer interval */
  itv.it_value.tv_sec = timer_time;			/* Set the timer time (seconds) */
  ec_neg1( setitimer(ITIMER_REAL, &itv, NULL) );	/* Set the timer */
  //while (true)
  while (1)
  {
    /*  add timer code here */
    /*  p. 659, Advanced Unix Programming */
    
    // test messages to verify timer is working
    printf("In [Timer] - flooder process status: is %srunning...\n", IsRunning == 1 ? "" : "not ");
    printf("In [Timer] - flooding status: is %sflooding...\n", IsFlooding == 1 ? "" : "not ");

    // Verify pid for current flooding process
    printf("In [Timer] - check [current_pid]: %d\n", current_pid);

    /* Action to take when the timer fires */
    printf("In [Timer] - called from Timer()...\n");

    break;
  } 
  return;
}

void Handler(int signum)
{
  int result;

  /* Action to take when the timer fires */
  printf("In [Handler] - called from Timer()...\n");

  //printf("In [Handler] - pid: %d\n", getpid());

  //printf("In [Handler] - check FLOODER value...\n");
  //DisplayFlooder(FLOODER);

  printf("In [Handler] - flooder process status: is %srunning...\n", IsRunning == 1 ? "" : "not ");
  printf("In [Handler] - flooding status: is %sflooding...\n", IsFlooding == 1 ? "" : "not ");

  printf("In [Handler] - pid: %d\n", getpid());

  if (IsRunning)
  {
    printf("In [Handler] - initializing result...\n");
    result = 0;

    if (IsFlooding)
    {
      // End of running interval - stop flooding
      printf("In [Handler] - (flooding) - end of flooding interval\n");
      printf("In [Handler] - stop flooding (killing [current_pid]...)\n");
      result = KillCurrentPid();
      if (result == 0)
      {
       printf("In [Handler] - [current_pid] successfully killed\n");
        //IsFlooding = false;
        IsFlooding = 0;
        printf("In [Handler] - IsFlooding = %s\n", IsFlooding == 1 ? "true" : "false");
      }
      else
      {
        printf("In [Handler] - error killing [current_pid]: %d\n", result);
        //IsFlooding = false;
        IsFlooding = 0;
        printf("In [Handler] - IsFlooding = %s\n", IsFlooding == 1 ? "true" : "false");
      }
    }
    else
    {
      // End of not-flooding interval - start flooding
      printf("In [Handler] - (not flooding) - end of not-flooding interval\n");
      printf("In [Handler] - start flooding (starting child pid...)\n");
      result = CreateChild(FLOODER);
      // result should contain child pid...
      if (result != 0)
      {
        printf("In [Handler] - child pid successfully started (pid %d)\n", result);

        // Assign child pid to current_pid
        current_pid = result;

        printf("In [Handler] - assigning child pid to [current_pid]: %d\n", current_pid);
        IsFlooding = 1;

        printf("In [Handler] - IsFlooding = %s\n", IsFlooding == 1 ? "true" : "false");
        //IsFlooding = true;
      }
      else
      {
        printf("In [Handler] - error creating child pid\n");
        printf("In [Handler] - creating child pid returned: %d\n", result);
      }
    }
  }
  
  printf("In [Handler] - done\n");
}


/*
 *
 * Main Program
 *
 */

int main()
{
  char cwd[1024];
  int status;

  printf("In [Main] - flooder program starting\n");
  printf("In [Main] - pid: %d\n", getpid());

  printf("Initializing log and error files\n");
  system("> /home/jim/cowe/flooder.log");
  system("> /home/jim/cowe/error.log");

  //while(true)
  while(1)
  {
    // Check the listen directory for a file
    printf("Checking for files...\n");
    int isEmpty =  IsDirectoryEmpty(LISTEN_DIR);
    if (isEmpty)
      printf("In [Main] - directory 'listen' is empty\n");
    else
      printf("In [Main] - directory 'listen' is not empty\n");

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
          printf("In [Main] - found 'start' file...\n");

          //FLOODER = ParseStartFile(); 
          ParseStartFile(); 

          /*
          strncpy(FLOODER.sipaddr, "10.10.10.208\0", 13);
          strncpy(FLOODER.dipaddr, "10.10.10.9\0", 11);
          strncpy(FLOODER.sportno, "403\0", 4);
          strncpy(FLOODER.dportno, "80\0", 3);
          strncpy(FLOODER.tinterval, "20\0", 3);
          */

          printf("In [Main] - check FLOODER after ParseStartFile ...\n");
          DisplayFlooder(FLOODER);

          //// Move the start file up one level to persist it
          //system("/bin/mv start ../start");

          // Delete the file 
          if (remove(START_FILE) != 0)
            perror("Error deleting 'start' file\n");

          // Start the flooder process
          //IsRunning = true;
          IsRunning = 1;

          printf("In [Main] - start flooding (calling CreateChild()...)\n");
          int result = CreateChild();
          if (result != 0)
          {
            // CreateChild() will return the child pid if successful - need to assign
            // this pid to [current_pid]; assignment in child pid won't work becuase
            // [current_pid] in child process belongs to different process.
            printf("In [Main] - child pid successfully started (pid %d)\n", getpid());
            //IsFlooding = true;
            IsFlooding = 1;
            
            printf("In [Main] - assign child pid to [current_pid]...\n");
            current_pid = result;
            printf("In [Main] - verify [current_pid]: %d\n", current_pid);
          }
          else
          {
            printf("In [Main] - error starting child pid - returned result: %d\n", result);
            printf("In [Main] - child pid successfully started (pid %d)\n", getpid());
            IsFlooding = 1;
            printf("In [Main] - error starting child pid - start flooding: %s\n", IsFlooding == 1 ? "true" : "false");
          }

          //printf("In [Main] - starting timer...\n");
          int time_interval = atoi(FLOODER.tinterval);
          printf("In [Main] - starting timer...running for %d seconds\n", time_interval);
          Timer(time_interval, time_interval, FLOODER);

          break;

        case 2:
          printf("In [Main] - found 'stop' file...\n");
          // Delete the file 
          if (remove(STOP_FILE) != 0)
            perror("Error deleting 'stop' file\n");

          IsRunning = 0;

          //printf("In [Main] - killing child pid %d\n", getpid());
          printf("In [Main] - killing child pid [current_pid] %d\n", current_pid);
          if (KillChildPid(current_pid) == 0)
          {
            printf("In [Main] - child pid successfully killed\n");
            //IsFlooding = false;
            IsFlooding = 0;
          }
          else
            printf("In [Main] - error killing child pid\n");
          break;

        default:
          printf("In [Main] - found unknown file - deleting file...\n");
          // Delete the file 
          if (remove(UNKNOWN_FILE) != 0)
            perror("In [Main] - error deleting unknown file\n");
          break;
      }
    }

    sleep(2); 

  }

  exit(0);
}

