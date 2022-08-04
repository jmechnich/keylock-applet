#include "Indicator.hh"

#include <X11/XKBlib.h>

#include <QDesktopWidget>

#include <QAbstractNativeEventFilter>

#include <iostream>

class EventFilter : public QAbstractNativeEventFilter
{
public:
  bool nativeEventFilter(const QByteArray &eventType, void* message,
                         long* /* result */)
  {
    auto e(static_cast<const xcb_client_message_event_t*>(message));
    switch (e->response_type & ~0x80) {
    case XCB_BUTTON_PRESS:
    {
      /* Handle the ButtonPress event type */
      std::cout << "Button Press" << std::endl;
      xcb_button_press_event_t *ev = (xcb_button_press_event_t *)e;
      break;
    }
    case XCB_KEY_PRESS:
    {
      /* Handle the ButtonPress event type */
      std::cout << "Key Press" << std::endl;
      xcb_key_press_event_t *ev = (xcb_key_press_event_t *)e;
      break;
    }
    case XCB_KEY_RELEASE:
    {
      /* Handle the ButtonPress event type */
      std::cout << "Key Release" << std::endl;
      xcb_key_release_event_t *ev = (xcb_key_release_event_t *)e;
      break;
    }
    default:
      std::cout << std::hex << int(e->response_type & ~0x80) << std::endl;
      break;
    }
    return false;
  }

  static EventFilter instance;
};
EventFilter EventFilter::instance;

Indicator::Indicator()
        : QSystemTrayIcon()
        , _p(new Preferences)
        , _s(new SplashScreen)
        , _states()
        , _win(0)
{
  initVars();
  initContextMenu();
  initSystray();
  connect( _p, SIGNAL( update()), this, SLOT( resetIcon()));
  connect( this, SIGNAL( activated( QSystemTrayIcon::ActivationReason)),
           this, SLOT( clickAction( QSystemTrayIcon::ActivationReason)));
  connect( QApplication::desktop(), SIGNAL(resized(int)),
           this, SLOT(screenSizeChanged(int)));
  initXkbExtension();
  QTimer::singleShot(10, this, SLOT( resetIcon()));
}

Indicator::~Indicator()
{
  delete _s;
  delete _p;
}

void
Indicator::resetIcon()
{
  int show_key = _p->value("show_key").toInt();
  if( show_key != ANY)
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
  map_iterator it = _map.find(show_key);
  if( it != _map.end() && it->action) it->action->setChecked( true);
  updateSplash();
}

void
Indicator::setIndicators(unsigned int changed, unsigned int state)
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
  if( hasChanged || show_key == ANY)
      resetIcon();
  else
      updateSplash();
}

bool
Indicator::initXkbExtension()
{
  int code;
  int maj = XkbMajorVersion;
  int min = XkbMinorVersion;
  int XkbErrorBase;

  _win = QX11Info::display();
  if (!XkbLibraryVersion(&maj, &min))
      return false;

  if (!XkbQueryExtension(_win, &code, &_XkbEventBase, &XkbErrorBase,
                         &maj, &min))
      return false;

  if (!XkbUseExtension(_win, &maj, &min))
      return false;

  if (!XkbSelectEvents(_win, XkbUseCoreKbd, XkbIndicatorStateNotifyMask,
                       XkbIndicatorStateNotifyMask))
      return false;

  unsigned int state;
  XkbGetIndicatorState(_win, XkbUseCoreKbd, &state);
  for( int bit = 0; bit < 3; ++bit)
  {
    _states[bit] = state & (1 << bit);
  }

  connect( this, SIGNAL(indicatorsChanged(unsigned int,unsigned int)),
           this, SLOT(setIndicators(unsigned int,unsigned int)));
  return true;
}

QString
Indicator::iconName( unsigned int key, unsigned int state)
{
  QString ret="caps-lock";
  map_const_iterator it = _map.find(key);
  if( it != _map.end())
      ret = it->iconName;

  if(state) ret += "-on";
  else      ret += "-off";

  return ret;
}

void
Indicator::initVars()
{
  QPixmap pix(22,22);
  pix.fill(Qt::black);
  _icon = QIcon(pix);

  _map.clear();
  _map.insert(CAPS,   Mode("Caps lock",   "CAPS", "caps-lock"));
  _map.insert(NUM,    Mode("Num lock",    "NUM",  "num-lock"));
  _map.insert(SCROLL, Mode("Scroll lock", "SCRL", "scroll-lock"));
  _map.insert(ANY,    Mode("All",         "",     ""));
}

void
Indicator::initContextMenu()
{
  QMenu* m = new QMenu;
  QAction* a = 0;
  QActionGroup* g = new QActionGroup(m);
  g->setExclusive(true);
  QSignalMapper* sm = new QSignalMapper(this);
  map_iterator it;
  for( int i=0; i<NKEYS; ++i)
  {
    it = _map.find(i);
    if( it == _map.end()) continue;

    // a = m->addAction(QIcon::fromTheme(mode.iconName+"-off"),
    //                  mode.longName);
    // QIcon icon;
    // icon.addPixmap( QIcon::fromTheme(it->iconName+"-off").pixmap(22,22),
    //                 QIcon::Normal, QIcon::Off);
    // icon.addPixmap( QIcon::fromTheme(it->iconName+"-on").pixmap(22,22),
    //                 QIcon::Normal, QIcon::On);
    a = g->addAction( it->longName);
    a->setCheckable(true);
    m->addAction(a);
    it->action = a;
    connect( a, SIGNAL(triggered()), sm, SLOT(map()));
    sm->setMapping( a, i);
  }
  connect( sm, SIGNAL( mapped( int)), this, SLOT( resetIcon( int)));
  m->addSeparator();
  m->addAction(QIcon::fromTheme("document-properties"), "&Preferences",
                   this, SLOT(preferences()));
  m->addAction(QIcon::fromTheme("application-exit"), "&Quit",
                   qApp, SLOT(quit()));
  setContextMenu(m);
}

void
Indicator::initSystray()
{
  _p->setWindowIcon(_icon);
  setIcon(_icon);
  show();
}

QAbstractNativeEventFilter* Indicator::eventFilter()
{
  return &EventFilter::instance;
}

void
Indicator::updateIcon()
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
  map_const_iterator it;
  for( int i=0; i<ANY; ++i)
  {
    it = _map.find(i);
    if( it == _map.end()) continue;

    QRect r( 0, 0, segh, segw);
    p.fillRect( r, _states[i] ? fgColor : bgColor);
    p.setPen( _states[i] ? bgColor : fgColor);
    p.save();
    p.scale(0.5, 0.5);
    p.drawText( r.left(), r.top(), r.width()*2, r.height()*2,
                Qt::AlignCenter, it->shortName);
    p.restore();
    p.translate( 0, segw+2);
  }
  p.end();
  _icon = QIcon(pix);
}

void
Indicator::updateSplash()
{
  syslog( LOG_DEBUG, "DEBUG  Indicator::updateSplash()");
  if(!_s) return;

  bool visible = false;
  QString text;

  map_const_iterator it;
  for( int i=0; i<ANY; ++i)
  {
    it = _map.find(i);
    if( it == _map.end()) continue;

    if( _states[i])
    {
      visible = true;
      text += it->longName + " ";
    }
  }

  if( !visible)
  {
    if(  _s->isVisible())
    {
      syslog( LOG_DEBUG, "DEBUG  hiding splash");
      _s->hide();
    }

    return;
  }
  syslog( LOG_DEBUG, "DEBUG  showing splash");
  _s->setText( text + "ON", _p->value("splash_font").value<QFont>());
  _s->show();
  _s->update();
}
