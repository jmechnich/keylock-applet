#include "EventFilter.hh"

#include <iostream>
#include <memory>

#include <QByteArray>

#include <syslog.h>

#include <X11/Xlib-xcb.h>
#include <xcb/xcb_event.h>
#define explicit _explicit
#include <xcb/xkb.h>
#undef explicit
#include <xcb/xinput.h>

EventFilter::EventFilter(Display* win)
        : QObject()
        , QAbstractNativeEventFilter()
        , _xcbConnection(XGetXCBConnection(win))
{
  initXkb();
  initXInput2();
}

bool
EventFilter::nativeEventFilter(const QByteArray &eventType, void* message,
                               long* /* result */)
{
  if (eventType == "xcb_generic_event_t") {
    xcb_generic_event_t* e = static_cast<xcb_generic_event_t*>(message);
    uint8_t response = XCB_EVENT_RESPONSE_TYPE(e);

    switch (response) {
    case XCB_KEY_PRESS:        //  2
    case XCB_KEY_RELEASE:      //  3
    case XCB_BUTTON_PRESS:     //  4
    case XCB_ENTER_NOTIFY:     //  7
    case XCB_LEAVE_NOTIFY:     //  8
    case XCB_EXPOSE:           // 12
    case XCB_UNMAP_NOTIFY:     // 18
    case XCB_MAP_NOTIFY:       // 19
    case XCB_REPARENT_NOTIFY:  // 21
    case XCB_CONFIGURE_NOTIFY: // 22
    case XCB_PROPERTY_NOTIFY:  // 28
    case XCB_CLIENT_MESSAGE:   // 33
    {
      //ignore
      break;
    }
    case XCB_GE_GENERIC: // 35
    {
      auto *ev = reinterpret_cast<xcb_ge_generic_event_t*>(e);
      if (_xi2Enabled && ev->extension == _xiOpCode) {
#ifndef QT_NO_DEBUG
        std::cout << "XINPUT event" << std::endl;
#endif
      } else {
#ifndef QT_NO_DEBUG
        std::cout << "Generic Event" << std::endl;
        std::cout << "Extension: " << int(ev->extension) << std::endl;
#endif
      }
      break;
    }
    default:
      if (response == _xkbFirstEvent) {
        uint8_t xkbType = e->pad0;
        switch (xkbType) {
        case XCB_XKB_NEW_KEYBOARD_NOTIFY:
        case XCB_XKB_STATE_NOTIFY:
        {
          break;
        }
        case XCB_XKB_INDICATOR_STATE_NOTIFY:
        {
          auto *ev = reinterpret_cast<
              xcb_xkb_indicator_state_notify_event_t*>(e);
          emit indicatorsChanged(ev->stateChanged, ev->state);
          break;
        }
        default:
#ifndef QT_NO_DEBUG
          std::cout << "Unhandled XKEYBOARD Event" << std::endl;
          std::cout << "  xkb type " << int(xkbType) << std::endl;
#endif
          break;
        }
        break;
      }
#ifndef QT_NO_DEBUG
      std::cout << "Unhandled event " << int(response) << std::endl;
      const char* label = xcb_event_get_label(response);
      if (label) {
        std::cout << label << std::endl;
      }
#endif
      break;
    }
  } else {
#ifndef QT_NO_DEBUG
    std::cout << "Unhandled event of type "
              << eventType.toStdString() << std::endl;
#endif
  }

  return false;
}

bool
EventFilter::initXkb()
{
  const xcb_query_extension_reply_t *reply =
      xcb_get_extension_data(_xcbConnection, &xcb_xkb_id);
  if(!reply || !reply->present) {
    syslog(
        LOG_DEBUG, "DEBUG  XKEYBOARD extension is not present on the X server");
    return false;
  }

  syslog(LOG_DEBUG, "DEBUG  Using XKB version %d.%d",
         XCB_XKB_MAJOR_VERSION, XCB_XKB_MINOR_VERSION);

  _xkbEnabled = true;
  _xkbOpCode = reply->major_opcode;
  _xkbFirstEvent = reply->first_event;

  syslog(LOG_DEBUG, "DEBUG    opcode %d", _xkbOpCode);
  syslog(LOG_DEBUG, "DEBUG    first event %d", _xkbFirstEvent);
  return true;
}

bool
EventFilter::initXInput2()
{
  const xcb_query_extension_reply_t *xinput =
      xcb_get_extension_data(_xcbConnection, &xcb_input_id);
  if(!xinput || !xinput->present) {
    syslog(LOG_DEBUG, "DEBUG  XInput extension is not present on the X server");
    return false;
  }

  auto cookie = xcb_input_xi_query_version(_xcbConnection, 2, 2);
  std::unique_ptr<xcb_input_xi_query_version_reply_t> reply(
      xcb_input_xi_query_version_reply(_xcbConnection, cookie, nullptr));
  if(!reply || reply->major_version != 2) {
    syslog(LOG_DEBUG, "DEBUG  XInput extension is not present on the X server");
    return false;
  }

  syslog(LOG_DEBUG, "DEBUG  Using XInput version %d.%d",
         reply->major_version, reply->minor_version);

  _xi2Enabled = true;
  _xiOpCode = xinput->major_opcode;
  _xiFirstEvent = xinput->first_event;
  _xi2Minor = reply->minor_version;

  syslog(LOG_DEBUG, "DEBUG    opcode %d", _xiOpCode);
  syslog(LOG_DEBUG, "DEBUG    first event %d", _xiFirstEvent);

  return true;
}
