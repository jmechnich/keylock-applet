#ifndef INDICATOR_HH
#define INDICATOR_HH

#include <QSystemTrayIcon>
#include <QSettings>
#include <QString>
#include <QMenu>
#include <QApplication>
#include <QX11Info>
#include <QPainter>
#include <QTimer>
#include <QActionGroup>

#include "Preferences.hh"
#include "SplashScreen.hh"

#include <X11/XKBlib.h>

class Indicator : public QSystemTrayIcon
{
  Q_OBJECT

  struct Mode
  {
    QString longName;
    QString shortName;
    QString iconName;
    QAction* action;
        
    Mode( const QString& l, const QString& s, const QString& i)
            : longName( l), shortName( s), iconName( i), action(0)
          {}
  };
  
public:
  enum
  {
      CAPS   = 0,
      NUM    = 1,
      SCROLL = 2,
      ANY    = 3,
      NKEYS  = 4
  };

  Indicator();

  ~Indicator()
        {
          delete _p;
        }

private slots:
  void clickAction( QSystemTrayIcon::ActivationReason)// r)
        {
          // printf( "clickAction: %d\n", r);
        }
  
  void screenSizeChanged( int /*screen*/)
        {
          QTimer::singleShot(1000, this, SLOT( resetIcon()));
        }
  
  void resetIcon();
  
  void resetIcon( int type)
        {
          _p->setValue("show_key", type);
          resetIcon();
        }

  void preferences()
        {
          _p->show();
        }
  
  void setIndicators(unsigned int changed, unsigned int state);
  bool initXkbExtension();

private:
  QString iconName( unsigned int key, unsigned int state);
  
  void initVars();
    
  void initContextMenu();
  
  void initSystray();
      
  void x11EventFilter(XEvent* event);
  
  void updateIcon();
    
  void updateSplash();
    
signals:
  void indicatorsChanged(unsigned int, unsigned int);
  
private:
  typedef QMap<unsigned int,Mode> map_type;
  typedef map_type::const_iterator map_const_iterator;
  typedef map_type::iterator map_iterator;
  map_type _map;
  
  Preferences* _p;
  SplashScreen* _s;
  int        _states[3];
  Display* _win;
  int _XkbEventBase;
  QIcon _icon;
  
  friend class IndicatorApplication;
};

#endif
