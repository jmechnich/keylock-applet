#ifndef SPLASHSCREEN_HH
#define SPLASHSCREEN_HH

#include <QWidget>
#include <QPaintEvent>
#include <QDesktopWidget>

class SplashScreen : public QWidget
{
public:
  SplashScreen()
          : QWidget()
        {
          setAttribute(Qt::WA_TranslucentBackground);
          setWindowFlags(Qt::SplashScreen|Qt::WindowStaysOnTopHint);
          setGeometry(0,0,1,1);
        }

  void setText( const QString& text, const QFont& font)
        {
          _text = text;
          _font = font;
        }

private:
  void paintEvent ( QPaintEvent* event)
        {
          QFontMetrics fm(_font);
          QRect fr(fm.boundingRect(_text));
          int margin = 2, spacing=20;
          int w = fr.width()+2*spacing, h = fr.height()+2*spacing;
          QRect sr( QApplication::desktop()->availableGeometry());
          setGeometry( sr.width()-w-margin, sr.top()+margin, w, h);

          QColor fgColor("#33b0dc"), bgColor("#144556");
          QPainter p(this);
          p.setPen(fgColor);
          p.setFont(_font);
          p.drawText( 0, 0, w, h,  Qt::AlignCenter, _text);
          p.end();
          event->accept();
        }

private:
  QString _text;
  QFont   _font;
};

#endif
