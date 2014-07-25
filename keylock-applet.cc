#include "IndicatorApplication.hh"

#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>

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

void setLogLevel( int level, bool isDaemon)
{
  int option = LOG_PID;
  if( !isDaemon) option |= LOG_PERROR;
  
  openlog("keylock-applet", option, LOG_USER);
  switch( level)
  {
  case 1:
    setlogmask( LOG_UPTO( LOG_NOTICE));
    break;
  case 2:
    setlogmask( LOG_UPTO( LOG_INFO));
    break;
  case 3:
    setlogmask( LOG_UPTO( LOG_DEBUG));
    break;
  default:
    setlogmask( LOG_UPTO( LOG_WARNING));
    break;
  }
}

void help()
{
  printf( "usage: keylock-applet [options]\n");
  printf( "\n");
  printf( "  short long         description\n");
  printf( "  -d    --daemon     start as daemon\n");
  printf( "  -h    --help       print this help and exit\n");
  printf( "  -v N  --verbose N  set syslog verbosity level to N.\n");
  printf( "                     Values of N can be:\n");
  printf( "                        1  LOG_NOTICE\n");
  printf( "                        2  LOG_INFO\n");
  printf( "                        3  LOG_DEBUG\n");
}

void parseCommandLine( int argc, char** argv)
{
  bool daemon = false;
  int verbosity = 0;
  int c = 0;
  while( true)
  {
    static struct option long_options[] =
        {
            {"daemon",  no_argument,       0, 'd'},
            {"help",    no_argument,       0, 'h'},
            {"verbose", required_argument, 0, 'v'},
            {0, 0, 0, 0},
        };
    
    int option_index = 0;
    c = getopt_long (argc, argv, "dhv:", long_options, &option_index);
    if( c == -1) break;
    switch (c)
    {
    case 0:
      /* If this option set a flag, do nothing else now. */
      if (long_options[option_index].flag != 0)
          break;
      printf ("option %s", long_options[option_index].name);
      if (optarg)
          printf (" with arg %s", optarg);
      printf ("\n");
      break;
    case 'd':
        daemon = true;
        break;
      case 'v':
        verbosity = atoi(optarg);
        break;
      case 'h':
        help();
        exit(0);
        break;
      default:
        abort();
        break;
    }
  }
  

  if( daemon)    detach();
  setLogLevel( verbosity, daemon);
  
  if (optind < argc)
  {
    printf ("unknown uptions: ");
    while (optind < argc)
        printf ("%s ", argv[optind++]);
    putchar ('\n');
    help();
    abort();
  }
}

void cleanup()
{
  closelog();
}

static int setupSignalHandlers()
{
  struct sigaction hup, term, inta;
  
  hup.sa_handler = IndicatorApplication::hupSignalHandler;
  sigemptyset(&hup.sa_mask);
  hup.sa_flags = 0;
  hup.sa_flags |= SA_RESTART;
  if( sigaction(SIGHUP, &hup, 0) > 0) return 1;
  
  term.sa_handler = IndicatorApplication::termSignalHandler;
  sigemptyset(&term.sa_mask);
  term.sa_flags = 0;
  term.sa_flags |= SA_RESTART;
  if( sigaction(SIGTERM, &term, 0) > 0) return 2;
  
  inta.sa_handler = IndicatorApplication::intSignalHandler;
  sigemptyset(&inta.sa_mask);
  inta.sa_flags = 0;
  inta.sa_flags |= SA_RESTART;
  if( sigaction(SIGINT, &inta, 0) > 0) return 3;

  return 0;
}

int main(int argc, char** argv)
{
  atexit( cleanup);

  parseCommandLine(argc, argv);
  IndicatorApplication a(argc, argv);

  if(setupSignalHandlers())
  {
    syslog( LOG_ERR, "ERROR  Could not set up signal handlers");
    return 1;
  }
    
  syslog( LOG_INFO, "INFO   Startup complete");
  return a.exec();
}
