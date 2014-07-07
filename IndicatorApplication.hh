#ifndef INDICATORAPPLICATION
#define INDICATORAPPLICATION

#include <QApplication>
#include <QMessageBox>
#include <QDirIterator>
#include <QApplication>

#include "Indicator.hh"

#include <unistd.h>

class IndicatorApplication : public QApplication
{
public:
  IndicatorApplication( int argc, char** argv);
  
  ~IndicatorApplication()
        {
          delete _i;
        }
  
private:
  bool x11EventFilter( XEvent* ev)
        {
          _i->x11EventFilter( ev);
          return false;
        }

  void setThemeFromGtk() const;

  Indicator* _i;
};

#endif
