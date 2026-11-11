/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/notification/notification_timeout_timers.h"

#include "base/location.h"
#include "base/logging.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_feature.h"

namespace brave_ads {

NotificationTimeoutTimers::NotificationTimeoutTimers() = default;

NotificationTimeoutTimers::~NotificationTimeoutTimers() = default;

void NotificationTimeoutTimers::Start(const std::string& notification_id,
                                      base::OnceClosure closure) {
  const base::TimeDelta timeout = kNotificationAdTimeout.Get();
  if (timeout.is_zero()) {
    // Never time out.
    return;
  }

  auto timer = std::make_unique<base::OneShotTimer>();
  timer->Start(FROM_HERE, timeout, std::move(closure));
  timers_by_notification_id_[notification_id] = std::move(timer);

  VLOG(6) << "Timeout for notification id " << notification_id << " in "
          << timeout;
}

bool NotificationTimeoutTimers::Stop(const std::string& notification_id) {
  return timers_by_notification_id_.erase(notification_id) > 0;
}

void NotificationTimeoutTimers::StopAll() {
  timers_by_notification_id_.clear();
}

}  // namespace brave_ads
