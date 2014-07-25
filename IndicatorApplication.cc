#include "IndicatorApplication.hh"

#include <QDateTime>

#include <sys/socket.h>
#include <syslog.h>

int IndicatorApplication::sighupFd[2]  = {0};
int IndicatorApplication::sigtermFd[2] = {0};
int IndicatorApplication::sigintFd[2]  = {0};

IndicatorApplication::IndicatorApplication( int argc, char** argv)
        : QApplication(argc, argv)
{
  initSignalHandlers();
  setThemeFromGtk();
                    
  // initialize general data
  setOrganizationName("mechnich");
  setApplicationName("keylock-applet");
  setApplicationVersion("1.0");
  setQuitOnLastWindowClosed(false);

  abortIfRunning();
  abortIfNoSystray();
  
  updatePreferences();
  
  _i = new Indicator;

  connect( this, SIGNAL( aboutToQuit()), this, SLOT( cleanup()));
}

IndicatorApplication::~IndicatorApplication()
{
  delete _i;
}

void
IndicatorApplication::abortIfRunning() const
{
  // abort if application is already running
  QRegExp re( "^\\d+$");
  QDirIterator it("/proc");
  while (it.hasNext())
  {
    QString name = it.next();
    QFileInfo fi( it.fileInfo());
    if( fi.isDir() && fi.isReadable() && re.exactMatch( fi.baseName()))
    {
      if( it.fileName().toInt() == getpid())
          continue;
              
      QFile file(QString(it.filePath() + "/cmdline"));
      if( !file.open(QIODevice::ReadOnly))
      {
        printf("Could not open file\n");
        continue;
      }
      QList<QByteArray> args = file.readAll().split('\0');
      if( args.size() && QString(args[0]).endsWith( applicationName()))
      {
        QMessageBox::critical(
            0, applicationName(), 
            QString("%1 is already running (pid %2)")
            .arg(applicationName()).arg( it.fileName()));
        abort();
      }
    }
  }
}

void
IndicatorApplication::abortIfNoSystray() const
{
  // abort if system tray is not available
  int retry = 3;
  while( retry--)
  {
    if( Indicator::isSystemTrayAvailable())
        break;
            
    if( !retry)
    {
      QMessageBox::critical(
          0, applicationName(), "System tray not available");
      abort();
    }
            
    sleep(3);
  }
}

void
IndicatorApplication::setThemeFromGtk() const
{
  QFile f(QDir::homePath() + "/.gtkrc-2.0");
  
  if( !f.open(QIODevice::ReadOnly | QIODevice::Text))
      return;
  while( !f.atEnd())
  {
    QByteArray l = f.readLine().trimmed();
    if( l.startsWith("gtk-icon-theme-name="))
    {
      QString s(l.split('=').back());
      QIcon::setThemeName(s.remove('"'));
      break;
    }
  }
}

void
IndicatorApplication::updatePreferences() const
{
#ifdef INSTALL_PREFIX
  QFileInfo distFile( QString(INSTALL_PREFIX) + "/share/" +
                      QApplication::applicationName() + "/" +
                      QApplication::applicationName() + ".conf");
  
  if( !distFile.exists())
      return;
  
  QFileInfo localFile( QDir::homePath() + "/.config/" +
                       QApplication::organizationName() + "/" +
                       QApplication::applicationName() +".conf");

  if( localFile.exists() && distFile.lastModified() > localFile.lastModified())
  {
    localFile.dir().remove(localFile.fileName());
    localFile.refresh();
  }
  
  if( !localFile.exists())
  {
    QFile::copy( distFile.absoluteFilePath(), localFile.absoluteFilePath());
  }
#endif
}

void
IndicatorApplication::initSignalHandlers()
{
  if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sighupFd))
      qFatal("Couldn't create HUP socketpair");
  if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sigtermFd))
      qFatal("Couldn't create TERM socketpair");
  if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sigintFd))
      qFatal("Couldn't create INT socketpair");
  
  snHup  = new QSocketNotifier(sighupFd[1],  QSocketNotifier::Read, this);
  connect(snHup,  SIGNAL(activated(int)), this, SLOT(handleSigHup()));
  snTerm = new QSocketNotifier(sigtermFd[1], QSocketNotifier::Read, this);
  connect(snTerm, SIGNAL(activated(int)), this, SLOT(handleSigTerm()));
  snInt  = new QSocketNotifier(sigintFd[1],  QSocketNotifier::Read, this);
  connect(snInt,  SIGNAL(activated(int)), this, SLOT(handleSigInt()));
}

void
IndicatorApplication::hupSignalHandler(int)
{
  char a = 1;
  ::write(sighupFd[0], &a, sizeof(a));
}

void
IndicatorApplication::termSignalHandler(int)
{
  char a = 1;
  ::write(sigtermFd[0], &a, sizeof(a));
}

void
IndicatorApplication::intSignalHandler(int)
{
  char a = 1;
  ::write(sigintFd[0], &a, sizeof(a));
}

void
IndicatorApplication::handleSigHup() const
{
  snHup->setEnabled(false);
  char tmp;
  ::read(sighupFd[1], &tmp, sizeof(tmp));
  
  syslog(LOG_INFO, "INFO   received SIGHUP");
  
  snHup->setEnabled(true);
}

void
IndicatorApplication::handleSigTerm() const
{
  snTerm->setEnabled(false);
  char tmp;
  ::read(sigtermFd[1], &tmp, sizeof(tmp));
  
  syslog(LOG_INFO, "INFO   received SIGTERM");
  
  snTerm->setEnabled(true);
}

void
IndicatorApplication::handleSigInt() const
{
  snInt->setEnabled(false);
  char tmp;
  ::read(sigintFd[1], &tmp, sizeof(tmp));
  
  syslog(LOG_INFO, "INFO   received SIGINT");
  
  quit();
  
  snInt->setEnabled(true);
}

void
IndicatorApplication::cleanup() const
{
  syslog( LOG_INFO, "INFO   Shutting down");
}
  
bool
IndicatorApplication::x11EventFilter( XEvent* ev)
{
  _i->x11EventFilter( ev);
  return false;
}
