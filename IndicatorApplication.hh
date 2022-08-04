#ifndef INDICATORAPPLICATION_HH
#define INDICATORAPPLICATION_HH

#include <QApplication>

class Indicator;

class IndicatorApplication : public QApplication
{
  Q_OBJECT

public:
  IndicatorApplication( int& argc, char** argv);
  ~IndicatorApplication();

private slots:
  void handleSignal(int signum);
  void cleanup() const;

private:
  void setThemeFromGtk() const;
  void abortIfRunning() const;
  void abortIfNoSystray() const;

  void updatePreferences() const;

  void initSignalHandlers();

  Indicator* _i;
};

#endif // INDICATORAPPLICATION_HH
