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
    int     bit;
    QString iconName;
    
    Mode( const QString& l, const QString& s, int b, const QString& i)
            : longName( l), shortName( s), bit( b), iconName( i)
          {}
  };
  
public:
  enum
  {
      CAPS=0,
      NUM=1,
      SCROLL=2
  };

  Indicator()
          : QSystemTrayIcon()
          , _p(new Preferences)
          , _s(new SplashScreen)
          , _states()
          , _win(0)
          , _icon(QPixmap(48,48))
        {
          initContextMenu();
          initSystray();
          connect( _p, SIGNAL( update()), this, SLOT( resetIcon()));
          connect( QApplication::desktop(), SIGNAL(resized(int)),
                   this, SLOT(screenSizeChanged(int)));
          initXkbExtension();
          QTimer::singleShot(10, this, SLOT( resetIcon()));
          _s->show();
        }

  ~Indicator()
        {
          delete _p;
        }

private slots:
  void screenSizeChanged( int /*screen*/)
        {
          QTimer::singleShot(1000, this, SLOT( resetIcon()));
        }
  
  void resetIcon()
        {
          
          int show_key = _p->value("show_key").toInt();
          if( show_key != -1)
          {
            QString prefix = _p->value("prefix").toString();
            QString suffix = _p->value("suffix").toString();
            QString iconPath = prefix + "/" +
                iconName(show_key,_states[show_key]) + "." + suffix;
            _icon = QIcon( iconPath);
          }
          else updateIcon();
          setIcon( _icon);
          _p->setWindowIcon(_icon);
          updateSplash();
        }
  void resetIcon( int type)
        {
          _p->setValue("show_key", type);
          resetIcon();
        }

  void preferences()
        {
          _p->show();
        }
  
  void setIndicators(unsigned int changed, unsigned int state)
        {
          int show_key = _p->value("show_key").toInt();
          bool hasChanged = false;
          for( int bit = 0; bit < 3; ++bit)
          {
            if (changed & (1 << bit))
            {
              if( show_key == bit) hasChanged = true;
              _states[bit] = state & (1 << bit);
            }
          }
          if( hasChanged || show_key == -1)
              resetIcon();
        }
  
  bool initXkbExtension()
        {
          int code;
          int maj = XkbMajorVersion;
          int min = XkbMinorVersion;
          int XkbErrorBase;
          
          _win = QX11Info::display();
          
          if (!XkbLibraryVersion(&maj, &min))
              return false;
          
          if (!XkbQueryExtension(_win, &code, &_XkbEventBase, &XkbErrorBase, &maj, &min))
              return false;
          
          if (!XkbUseExtension(_win, &maj, &min))
              return false;
          
          if (!XkbSelectEvents(_win, XkbUseCoreKbd, XkbIndicatorStateNotifyMask, XkbIndicatorStateNotifyMask))
              return false;
          
          unsigned int state;
          XkbGetIndicatorState(_win, XkbUseCoreKbd, &state);
          setIndicators( (1<<4)-1, state);
          
          connect( this, SIGNAL(indicatorsChanged(unsigned int,unsigned int)),
                   this, SLOT(setIndicators(unsigned int,unsigned int)));
          return true;
        }

private:
  QString iconName( unsigned int key, unsigned int state)
        {
          QString ret="caps-lock";
          for( int i=0; i<_list.size(); ++i)
          {
            const Mode& mode = _list.at(i);
            if(mode.bit == int(key))
            {
              ret = mode.iconName;
              break;
            }
          }

          if(state) ret += "-on";
          else      ret += "-off";
          
          return ret;
        }

  void initContextMenu()
        {
          _list.clear();
          _list.append( Mode("Caps lock",   "CAPS", CAPS,   "caps-lock"));
          _list.append( Mode("Num lock",    "NUM",  NUM,    "num-lock"));
          _list.append( Mode("Scroll lock", "SCRL", SCROLL, "scroll-lock"));
          _list.append( Mode("All",         "",     -1,     ""));
          
          QMenu* m = new QMenu;
          QAction* a = 0;
          QSignalMapper* sm = new QSignalMapper(this);
          for( int i=0; i<_list.size(); ++i)
          {
            const Mode& mode = _list.at(i);
            a = m->addAction(mode.longName);
            connect( a, SIGNAL(triggered()), sm, SLOT(map()));
            sm->setMapping( a, mode.bit);
          }
          connect( sm, SIGNAL( mapped( int)), this, SLOT( resetIcon( int)));
          m->addSeparator();
          m->addAction("&Preferences", this, SLOT(preferences()));
          m->addAction("&Quit", qApp, SLOT(quit()));
          setContextMenu(m);
        }

  void initSystray()
        {
          _p->setWindowIcon(_icon);
          setIcon(_icon);
          show();
          
        }
    
  void x11EventFilter(XEvent* event)
        {
          XkbEvent* xkbEvent = reinterpret_cast<XkbEvent*>(event);
          
          if (xkbEvent->type == _XkbEventBase + XkbEventCode)
              if (xkbEvent->any.xkb_type == XkbIndicatorStateNotify)
                  emit indicatorsChanged(xkbEvent->indicators.changed,
                                         xkbEvent->indicators.state);
        }

  void updateIcon()
        {
          QColor fgColor("#33b0dc"), bgColor("#144556");
          QPixmap pix(geometry().size());
          pix.fill(Qt::black);
          int w=pix.width(), h=pix.height(), segw = (w-4)/3, segh = (h-2);
          QPainter p( &pix);
          p.rotate(-90);
          p.translate(-h+1, 1);
          QFont f( _p->value("icon_font").value<QFont>());
          p.setFont(f);
          for( int i=0; i<3; ++i)
          {
            QRect r( 0, 0, segh, segw);
            p.fillRect( r, _states[i] ? fgColor : bgColor);
            p.setPen( _states[i] ? bgColor : fgColor);
            p.save();
            p.scale(0.5, 0.5);
            p.drawText( r.left(), r.top(), r.width()*2, r.height()*2,
                        Qt::AlignCenter, _list.at(i).shortName);
            p.restore();
            p.translate( 0, segw+2);
            
          }
          p.end();
          _icon = QIcon(pix);
        }
  
  void updateSplash()
        {
          bool visible = false;
          QString text;
          
          for( int i=0; i<_list.size(); ++i)
          {
            const Mode& mode = _list.at(i);
            if( mode.bit == -1) continue;
            
            if( _states[mode.bit])
            {
              visible = true;
              text += mode.longName + " ";
            }
          }
          
          if( !visible)
          {
            _s->hide();
            return;
          }
          _s->setText( text + "ON", _p->value("splash_font").value<QFont>());
          _s->show();
          _s->update();
        }
  
signals:
  void indicatorsChanged(unsigned int, unsigned int);
  
private:
  Preferences* _p;
  SplashScreen* _s;
  int        _states[3];
  Display* _win;
  int _XkbEventBase;
  QIcon _icon;
  QList<Mode> _list;
  
  friend class IndicatorApplication;
};

#endif
