#include "IndicatorApplication.hh"

#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>

void detach()
{
  /* If launched by init (process 1), there’s no need to detach.
   *
   * Note: this test is unreliable due to an unavoidable race
   * condition if the process is orphaned.
  */
  if (getppid() == 1)
      return;
  
  /* Ignore terminal stop signals */
#ifdef SIGTTOU
  signal(SIGTTOU, SIG_IGN);
#endif
#ifdef SIGTTIN
  signal(SIGTTIN, SIG_IGN);
#endif
#ifdef SIGTSTP
  signal(SIGTSTP, SIG_IGN);
#endif

  /* Allow parent shell to continue.
   * Ensure the process is not a process group leader.
   */
  if (fork() != 0)
      exit(0); /* parent */
  
  /* child */

  /* Disassociate from controlling terminal and process group.
   *
   * Ensure the process can’t reacquire a new controlling terminal.
   * This is done differently on BSD vs. AT&T:
   *
   * BSD won’t assign a new controlling terminal
   * because process group is non-zero.
   *
   * AT&T won’t assign a new controlling terminal
   * because process is not a process group leader.
   * (Must not do a subsequent setpgrp()!)
   */
#ifdef BSD
  setpgrp(0, getpid()); /* change process group */
  
  int fd;
  if ((fd = open("/dev/tty", O_RDWR)) >= 0)
  {
    ioctl(fd, TIOCNOTTY, 0); /* lose controlling terminal */
    close(fd);
  }
#else /* AT&T */
  setpgrp(); /* lose controlling terminal & change process group */
  
  signal(SIGHUP, SIG_IGN); /* immune from pgrp leader death */
  if (fork() != 0)         /* become non-pgrp-leader */
      exit(0);             /* first child */
  
  /* second child */
#endif

  for (int fd = 0; fd < sysconf(_SC_OPEN_MAX); fd++)
      close(fd); /* close all file descriptors */
  chdir("/"); /* move current directory off mounted filesystem */
  umask(0);   /* clear any inherited file mode creation mask */
  return;
}

void parseCommandLine( int argc, char** argv)
{
  bool daemon = false;

  int c = 0;
  while ((c = getopt (argc, argv, "d")) != -1)
      switch (c)
      {
      case 'd':
        daemon = true;
        break;
      default:
        abort();
        break;
      }

  if( daemon) detach();
}

int main(int argc, char** argv)
{
  parseCommandLine(argc, argv);
  IndicatorApplication a(argc, argv);
  return a.exec();
}
