#include "IndicatorApplication.hh"

IndicatorApplication::IndicatorApplication( int argc, char** argv)
        : QApplication(argc, argv)
{
  setThemeFromGtk();
                    
  // initialize general data
  setOrganizationName("mechnich");
  setApplicationName("keylock-applet");
  setApplicationVersion("1.0");
  setQuitOnLastWindowClosed(false);
          
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

  _i = new Indicator;
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
