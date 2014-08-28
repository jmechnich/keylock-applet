#include "SplashScreen.hh"

#include <QApplication>
#include <QPaintEvent>
#include <QPainter>
#include <QDesktopWidget>

#ifdef Q_WS_X11
#include <QX11Info>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#endif

SplashScreen::SplashScreen()
        : QWidget()
        , _color( "#33b0dc")
{
  setAttribute(Qt::WA_TranslucentBackground);
  setWindowFlags(Qt::SplashScreen|Qt::WindowStaysOnTopHint);
  setGeometry(0,0,1,1);
}

void
SplashScreen::setText( const QString& text, const QFont& font)
{
  syslog( LOG_DEBUG, "DEBUG  SplashScreen::setText: %s", text.toLatin1().constData());
  _text = text;
  _font = font;
}

void
SplashScreen::show()
{
  syslog( LOG_DEBUG, "DEBUG  SplashScreen::show");
  updateGeometry();
  QWidget::show();
#ifdef Q_WS_X11
  unsigned long data = 0xFFFFFFFF;
  XChangeProperty (QX11Info::display(),
                   winId(),
                   XInternAtom(QX11Info::display(), "_NET_WM_DESKTOP", False),
                   XA_CARDINAL,
                   32,
                   PropModeReplace,
                   reinterpret_cast<unsigned char *>(&data), // all desktop
                   1);
#endif
  syslog( LOG_DEBUG, "DEBUG  SplashScreen::show END");
}

void
SplashScreen::paintEvent( QPaintEvent* event)
{
  syslog( LOG_DEBUG, "DEBUG  SplashScreen::paintEvent");
  QPainter p(this);
  p.setPen(_color);
  p.setFont(_font);
  p.drawText( 0, 0, width(), height(),  Qt::AlignCenter, _text);
  p.end();
  event->accept();
}

void
SplashScreen::updateGeometry()
{
  QFontMetrics fm(_font);
  QRect fr(fm.boundingRect(_text));
  int margin = 2, spacing=20;
  int w = fr.width()+2*spacing, h = fr.height()+2*spacing;
  QRect sr( QApplication::desktop()->availableGeometry());
  int l=sr.width()-w-margin, t=sr.top()+margin;
  setGeometry( l, t, w, h);
  syslog( LOG_DEBUG, "DEBUG  SplashScreen::updateGeometry(): %d, %d, %d, %d",
          l, t, w, h);
}
