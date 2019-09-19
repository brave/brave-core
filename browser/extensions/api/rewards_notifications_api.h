/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_API_REWARDS_NOTIFICATIONS_API_H_
#define BRAVE_BROWSER_EXTENSIONS_API_REWARDS_NOTIFICATIONS_API_H_

#include "extensions/browser/extension_function.h"

namespace extensions {
namespace api {

class RewardsNotificationsAddNotificationFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("rewardsNotifications.addNotification", UNKNOWN)

 protected:
  ~RewardsNotificationsAddNotificationFunction() override;

  ResponseAction Run() override;
};

class RewardsNotificationsDeleteNotificationFunction
    : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("rewardsNotifications.deleteNotification", UNKNOWN)

 protected:
  ~RewardsNotificationsDeleteNotificationFunction() override;

  ResponseAction Run() override;
};

class RewardsNotificationsDeleteAllNotificationsFunction
    : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("rewardsNotifications.deleteAllNotifications", UNKNOWN)

 protected:
  ~RewardsNotificationsDeleteAllNotificationsFunction() override;

  ResponseAction Run() override;
};

class RewardsNotificationsGetNotificationFunction : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("rewardsNotifications.getNotification", UNKNOWN)

 protected:
  ~RewardsNotificationsGetNotificationFunction() override;

  ResponseAction Run() override;
};

class RewardsNotificationsGetAllNotificationsFunction
    : public ExtensionFunction {
 public:
  DECLARE_EXTENSION_FUNCTION("rewardsNotifications.getAllNotifications", UNKNOWN)

 protected:
  ~RewardsNotificationsGetAllNotificationsFunction() override;

  ResponseAction Run() override;
};

}  // namespace api
}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_API_REWARDS_NOTIFICATIONS_API_H_
