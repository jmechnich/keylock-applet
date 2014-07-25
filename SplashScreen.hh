#ifndef SPLASHSCREEN_HH
#define SPLASHSCREEN_HH

#include <QWidget>

#include <syslog.h>

class SplashScreen : public QWidget
{
public:
  SplashScreen();
  void setText( const QString& text, const QFont& font);
  void show();
  
private:
  void paintEvent( QPaintEvent* event);
  
private:
  QString _text;
  QFont   _font;
};

#endif
