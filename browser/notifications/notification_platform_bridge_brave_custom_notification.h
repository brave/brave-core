// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_NOTIFICATIONS_NOTIFICATION_PLATFORM_BRIDGE_BRAVE_CUSTOM_NOTIFICATION_H_
#define BRAVE_BROWSER_NOTIFICATIONS_NOTIFICATION_PLATFORM_BRIDGE_BRAVE_CUSTOM_NOTIFICATION_H_

#include "base/macros.h"
#include "chrome/browser/notifications/notification_platform_bridge.h"
#include "brave/ui/brave_custom_notification/public/cpp/notification.h"

class Profile;

// Implementation of the platform bridge that enables delivering notifications
// through Chrome's Message Center. Default bridge for Windows, fallback bridge
// for mac OS and Linux.
//
// Different from the other platform bridges, which are global to the process,
// the Message Center bridge will be created on demand by the notification
// display service and is thereby associated with a particular profile.
class NotificationPlatformBridgeBraveCustomNotification
    : public NotificationPlatformBridge {
 public:
  explicit NotificationPlatformBridgeBraveCustomNotification(Profile* profile);
  ~NotificationPlatformBridgeBraveCustomNotification() override;

  // NotificationPlatformBridge implementation:
  void Display(NotificationHandler::Type notification_type,
               Profile* profile,
               const message_center::Notification& notification,
               std::unique_ptr<NotificationCommon::Metadata> metadata) override;
  void Display(NotificationHandler::Type notification_type,
               Profile* profile,
               brave_custom_notification::Notification& notification,
               std::unique_ptr<NotificationCommon::Metadata> metadata);
  void Close(Profile* profile, const std::string& notification_id) override;
  void GetDisplayed(Profile* profile,
                    GetDisplayedNotificationsCallback callback) const override;
  void SetReadyCallback(NotificationBridgeReadyCallback callback) override;
  void DisplayServiceShutDown(Profile* profile) override;

 private:
  Profile* profile_;

  DISALLOW_COPY_AND_ASSIGN(NotificationPlatformBridgeBraveCustomNotification);
};

#endif
