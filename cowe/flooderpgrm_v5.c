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
const char LOG_FILE[] = "/home/jim/cowe/flooder.log";
const char ERROR_LOG_FILE[] = "/home/jim/cowe/error.log";
const char START_FILE[] = "start";
const char STOP_FILE[] = "stop";
char *LISTEN_DIR = "/home/jim/cowe/listen";
char *UNKNOWN_FILE;
pid_t pid;

typedef struct
{
  //char sipaddr[16];	// Source IP address (this flooder instance)	
  char * sipaddr;	// Source IP address (this flooder instance)	
  //char dipaddr[16];	// Destination IP address (where packets will be sent)
  char * dipaddr;	// Destination IP address (where packets will be sent)
  //int  sportno;	// Source port number (this flooder instance)
  char *  sportno;	// Source port number (this flooder instance)
  //int  dportno;	// Destination port number (where packets will be sent)
  char *  dportno;	// Destination port number (where packets will be sent)
  //int  tinterval;	// Time interval for running
  char * tinterval;	// Time interval for running
} flooder;


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

flooder ParseStartFile() 
{
  char cwd[1024];
  char fdata[41];
  //char timeinv[6];
  char *timeinv;
  flooder _Flooder;

  // Change to the listen directory
  if (strcmp(getcwd(cwd, sizeof(cwd)), LISTEN_DIR) != 0)
  {
    if (chdir(LISTEN_DIR) != 0)
      perror("Error changing to listening directory\n");
  }

  // Open the 'start' file and retrieve its contents
  FILE *infile;
  infile = fopen (START_FILE, "r");
  if (infile == NULL)
  {
    printf("Cannot open file '%s'\n", START_FILE);
    exit(8);
  }

  // Assuming a fixed size file: 
  //   Source IP address: 15
  //   Destination IP Address: 15
  //   Destination Port Number: 5 (not in current file)
  //   Time interval: 6
  //   Field separators ("|"): 3
  //   NULL character ("\0")
  fread(fdata, sizeof(char), 40, infile);

  fclose(infile);

  // Populate the flooder struct with the file data
  /*
  char coct[3];
  int noct;
  int i;
  int j = 0;
  int k = 0;
  int m = 0;
  int n = 0;
  int iszero = 1;

  for ( i = 0; i < 36; i++ )
  {
    if (i < 15)
    {
      coct[n] = fdata[i];
      if (i % 4 == 0)
      noct = atoi(coct);

      _Flooder.sipaddr[i] = fdata[i];
    }
    else if (i >= 15 && i < 30)
    {
      //printf("In [ParseStartFile] ... between i=15 and i<30; value of i:%d\n", i);
      _Flooder.dipaddr[i%15] = fdata[i];
    }
    else if (i >= 30 && i < 36)
    {
      timeinv[i%30] = fdata[i]; 
      //printf("In [ParseStartFile] ... between i=30 and i<36; value of i:%d\n", i);
    }
  }
  */

  // Use a tokenizer to parse the file
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
  //timeinv = p;

  //_Flooder.sipaddr[16] = '\0';
  //_Flooder.dipaddr[16] = '\0';
  //_Flooder.tinterval = atoi(timeinv);

  // Using fixed values for source and destination port numbers
  //_Flooder.dportno = 80;
  //_Flooder.sportno = 403;

  printf("In [ParseStartFile] - show flooder:\n");
  printf("   _Flooder.sipaddr:\t %s\n", _Flooder.sipaddr);
  printf("   _Flooder.dipaddr:\t %s\n", _Flooder.dipaddr);
  //printf("   _Flooder.dportno:\t %d\n", _Flooder.dportno);
  printf("   _Flooder.dportno:\t %s\n", _Flooder.dportno);
  //printf("   _Flooder.sportno:\t %d\n", _Flooder.sportno);
  printf("   _Flooder.sportno:\t %s\n", _Flooder.sportno);
  //printf("   _Flooder.tinterval:\t %d\n", _Flooder.tinterval);
  printf("   _Flooder.tinterval:\t %s\n", _Flooder.tinterval);

  return _Flooder;
}

//int CreateChild()
int CreateChild(flooder f)
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

      // Construct the packit injection string from the flooder
      printf("In [CreateChild] - constructing 'injector' string\n");
      //char injector[168];
      char injector[169];
      //char * injector;

      //sprintf(injector, "/usr/sbin/packit -m inject -t UDP -s %s -d %s -S %d -D %d -c %d -b 20 -p '0x 68 76 6D 72 20 74 65 73 74 20 75 70 64 20 70 61 63 6B 65 74 73'", f.sipaddr, f.dipaddr, f.sportno, f.dportno, f.tinterval);
      //sprintf(injector, "packit -m inject -t UDP -s %s -d %s -S %d -D %d -c %d -b 20 -p '0x 68 76 6D 72 20 74 65 73 74 20 75 70 64 20 70 61 63 6B 65 74 73'", f.sipaddr, f.dipaddr, f.sportno, f.dportno, f.tinterval);

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

      //execlp("/home/jim/udpinject.sh", "udpinject.sh", (char *) NULL);

      /*
      // Construct the packit injection string from the flooder
      printf("In [CreateChild] - constructing 'injector' string\n");
      //char injector[168];
      char injector[169];
      //char * injector;

      //sprintf(injector, "/usr/sbin/packit -m inject -t UDP -s %s -d %s -S %d -D %d -c %d -b 20 -p '0x 68 76 6D 72 20 74 65 73 74 20 75 70 64 20 70 61 63 6B 65 74 73'", f.sipaddr, f.dipaddr, f.sportno, f.dportno, f.tinterval);
      //sprintf(injector, "packit -m inject -t UDP -s %s -d %s -S %d -D %d -c %d -b 20 -p '0x 68 76 6D 72 20 74 65 73 74 20 75 70 64 20 70 61 63 6B 65 74 73'", f.sipaddr, f.dipaddr, f.sportno, f.dportno, f.tinterval);
       */

      // Start injecting packets
      printf("In [CreateChild] ...using 'injector' string:\n");
      printf("%s\n", injector);
      //system(injector);
      //execlp("/usr/sbin/packit", injector, (char *) NULL);

      //execlp("/usr/sbin/packit", "packit", "-m", "inject", "-t", "UDP", "-s", "10.10.10.208", "-d", "10.10.10.9", "-S", "403", "-D", "80", "-c", "300", "-b", "20", "-p", "'0x 68 76 6D 72 20 74 65 73 74 20 75 70 64 20 70 61 63 6B 65 74 73'", (char *) NULL);

      execlp("/usr/sbin/packit", "packit", "-m", "inject", "-t", "UDP", "-s", f.sipaddr, "-d", f.dipaddr, "-S", f.sportno , "-D", f.dportno, "-c", f.tinterval, "-b", "20", "-p", "'0x 68 76 6D 72 20 74 65 73 74 20 75 70 64 20 70 61 63 6B 65 74 73'", (char *) NULL);

      //system("packit -m inject -t UDP -s 10.10.10.208 -d 10.10.10.9 -S 403 -D 80 -c 300 -b 20 -p '0x 68 76 6D 72 20 74 65 73 74 20 75 70 64 20 70 61 63 6B 65 74 73'");

      fflush(stdout); close(out);
      fflush(stderr); close(err);

      dup2(save_out, fileno(stdout));
      dup2(save_err, fileno(stderr));

      close(save_out);
      close(save_err);

      waitpid(pid, &status, WNOHANG);
      printf("In [CreateChild] - child pid: %d\n", getpid());
      printf("In [CreateChild] - child waitpid status: %d\n",status);

      do
      {
        // Get pid exit status
        pid_t w = waitpid(getpid(), &cstatus, WUNTRACED | WCONTINUED);
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

      exit(EXIT_SUCCESS);
  }
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
          // Parse the file
          flooder f = ParseStartFile(); 

          // Delete the file 
          if (remove(START_FILE) != 0)
            perror("Error deleting 'start' file\n");

          //CreateChild();
          CreateChild(f);
          waitpid(-1, &status, WNOHANG);
          break;

        case 2:
          printf("...found 'stop' file...\n");
          // Delete the file 
          if (remove(STOP_FILE) != 0)
            perror("Error deleting 'stop' file\n");
          break;
          printf("Killing child pid %d\n", getpid());
          if (KillChildPid(getpid()) == 0)
            printf("Child pid successfully killed\n");
          else
            printf("Error killing child pid\n");
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

  }

  exit(0);
}

