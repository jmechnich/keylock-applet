#ifndef EVENTFILTER_HH
#define EVENTFILTER_HH

#include <QObject>
#include <QAbstractNativeEventFilter>

#include <X11/Xlib.h>

class xcb_connection_t;

class EventFilter : public QObject, public QAbstractNativeEventFilter
{
  Q_OBJECT

public:
  EventFilter(Display* parent);

  bool nativeEventFilter(const QByteArray &eventType, void* message, long*);

signals:
  void indicatorsChanged(unsigned int,unsigned int);

private:
  bool initXkb();
  bool initXInput2();

  xcb_connection_t* _xcbConnection = nullptr;

  bool _xi2Enabled = false;
  uint8_t _xi2Minor = 0;
  uint8_t _xiOpCode = 0;
  uint8_t _xiFirstEvent = 0;

  bool _xkbEnabled = false;
  uint8_t _xkbOpCode = 0;
  uint8_t _xkbFirstEvent = 0;
};

#endif // EVENTFILTER_HH
