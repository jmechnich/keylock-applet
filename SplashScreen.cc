#include "SplashScreen.hh"

#include <QApplication>
#include <QPaintEvent>
#include <QPainter>
#include <QDesktopWidget>

#ifdef Q_WS_X11 //only define on Qt 4.X 
#include <QX11Info> //Only on Qt 4.X , return expected in Qt 5.1
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#endif

SplashScreen::SplashScreen()
        : QWidget()
{
  setAttribute(Qt::WA_TranslucentBackground);
  setWindowFlags(Qt::SplashScreen|Qt::WindowStaysOnTopHint);
  setGeometry(0,0,1,1);

}

void
SplashScreen::setText( const QString& text, const QFont& font)
{
  _text = text;
  _font = font;
}

void
SplashScreen::show()
{
  QWidget::show();
#ifdef Q_WS_X11 //only define on Qt 4.X 
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
}

void
SplashScreen::paintEvent ( QPaintEvent* event)
{
  QFontMetrics fm(_font);
  QRect fr(fm.boundingRect(_text));
  int margin = 2, spacing=20;
  int w = fr.width()+2*spacing, h = fr.height()+2*spacing;
  QRect sr( QApplication::desktop()->availableGeometry());
  int l=sr.width()-w-margin, t=sr.top()+margin;
  setGeometry( l, t, w, h);
  syslog( LOG_DEBUG, "DEBUG  Splash geometry: %d, %d, %d, %d", l, t, w, h);
  QColor fgColor("#33b0dc"), bgColor("#144556");
  QPainter p(this);
  p.setPen(fgColor);
  p.setFont(_font);
  p.drawText( 0, 0, w, h,  Qt::AlignCenter, _text);
  p.end();
  event->accept();
}
