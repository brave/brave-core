/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_NOTIFICATION_NOTIFICATION_TIMEOUT_TIMERS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_NOTIFICATION_NOTIFICATION_TIMEOUT_TIMERS_H_

#include <memory>
#include <string>

#include "base/containers/flat_map.h"
#include "base/functional/callback.h"

namespace base {
class OneShotTimer;
}  // namespace base

namespace brave_ads {

// Starts and tracks per-notification-id timeout timers for displayed
// notifications, honoring `kNotificationAdTimeout`. Shared so that every
// consumer that shows a notification (notification ads, reminders, etc.)
// times it out the same way instead of each keeping its own timer map.
class NotificationTimeoutTimers final {
 public:
  NotificationTimeoutTimers();

  NotificationTimeoutTimers(const NotificationTimeoutTimers&) = delete;
  NotificationTimeoutTimers& operator=(const NotificationTimeoutTimers&) =
      delete;

  ~NotificationTimeoutTimers();

  // Starts a timeout timer for `notification_id`, running `closure` if it
  // fires. Does nothing if `kNotificationAdTimeout` is zero (never time
  // out).
  void Start(const std::string& notification_id, base::OnceClosure closure);

  // Returns `true` if a timer for `notification_id` was running and has been
  // cancelled.
  bool Stop(const std::string& notification_id);

  void StopAll();

 private:
  base::flat_map<std::string, std::unique_ptr<base::OneShotTimer>>
      timers_by_notification_id_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_NOTIFICATION_NOTIFICATION_TIMEOUT_TIMERS_H_
