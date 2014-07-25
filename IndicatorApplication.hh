#ifndef INDICATORAPPLICATION
#define INDICATORAPPLICATION

#include <QApplication>
#include <QMessageBox>
#include <QDirIterator>
#include <QApplication>
#include <QSocketNotifier>

#include "Indicator.hh"

#include <unistd.h>

class IndicatorApplication : public QApplication
{
  Q_OBJECT

public:
  IndicatorApplication( int argc, char** argv);
  
  ~IndicatorApplication();
  
  static void hupSignalHandler(int);
  static void termSignalHandler(int);
  static void intSignalHandler(int);

public slots:
  // Qt signal handlers.
  void handleSigHup() const;
  void handleSigTerm() const;
  void handleSigInt() const;
  
private slots:
  void cleanup() const;

private:
  bool x11EventFilter( XEvent* ev);

  void setThemeFromGtk() const;
  void abortIfRunning() const;
  void abortIfNoSystray() const;
  
  void updatePreferences() const;

  void initSignalHandlers();
  
  static int sighupFd[2];
  static int sigtermFd[2];
  static int sigintFd[2];
  
  QSocketNotifier* snHup;
  QSocketNotifier* snTerm;
  QSocketNotifier* snInt;
  
  Indicator* _i;
};

#endif
