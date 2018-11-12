#ifndef CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_HANDLER_OVERRIDE_H_
#define CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_HANDLER_OVERRIDE_H_

#define NotificationHandler NotificationHandler_ChromiumImpl
#include "../../../../../../chrome/browser/notifications/notification_handler.h"
#undef NotificationHandler

class NotificationHandler : public NotificationHandler_ChromiumImpl {
 public:
  enum class Type {
    WEB_PERSISTENT = 0,
    WEB_NON_PERSISTENT = 1,
    EXTENSION = 2,
    TRANSIENT = 3,
    BRAVE_ADS = 4,
    MAX = BRAVE_ADS,
  };
};

#endif  // CHROME_BROWSER_NOTIFICATIONS_NOTIFICATION_HANDLER_OVERRIDE_H_
