#ifndef INDICATORAPPLICATION
#define INDICATORAPPLICATION

#include <QApplication>
#include <QMessageBox>
#include <QDirIterator>
#include <QTextStream>

#include "Indicator.hh"

#include <unistd.h>

class IndicatorApplication : public QApplication
{
public:
  IndicatorApplication( int argc, char** argv)
          : QApplication(argc, argv)
        {
          // initialize general data
          setOrganizationName("mechnich");
          setApplicationName("indicator-keylock-qt");
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
  ~IndicatorApplication()
        {
          delete _i;
        }
  
  bool x11EventFilter( XEvent* ev)
        {
          _i->x11EventFilter( ev);
          return false;
        }

private:
  Indicator* _i;
};

#endif
