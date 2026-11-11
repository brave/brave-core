/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_REMINDER_REMINDER_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_REMINDER_REMINDER_MANAGER_H_

#include <optional>
#include <string>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/browser/notification/notification_timeout_timers.h"
#include "brave/components/brave_ads/browser/reminder/reminder_info.h"
#include "brave/components/brave_ads/core/browser/service/ads_service.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace brave_ads {

// Shows reminders and recognizes reminder notification events, so that
// `AdsServiceImpl` never has to know a given notification is a reminder
// rather than a real notification ad.
class ReminderManager final {
 public:
  explicit ReminderManager(AdsService::Delegate& delegate);

  ReminderManager(const ReminderManager&) = delete;
  ReminderManager& operator=(const ReminderManager&) = delete;

  ~ReminderManager();

  void MaybeShow(mojom::ReminderType mojom_reminder_type);

  // Returns `true` if `placement_id` belongs to the currently shown reminder.
  bool IsShowingReminder(const std::string& placement_id) const;

  // Each returns `true` if `placement_id` belongs to a reminder, in which case
  // the event has been fully handled and must not be forwarded to the ads core.
  bool MaybeHandleClosed(const std::string& placement_id);
  bool MaybeHandleClicked(const std::string& placement_id);

  // Call on shutdown so a pending reminder cannot time out and call into the
  // delegate after it has started tearing down.
  void Shutdown();

 private:
  void TimedOut(const std::string& placement_id);

  const raw_ref<AdsService::Delegate> delegate_;

  std::optional<ReminderInfo> shown_reminder_;
  NotificationTimeoutTimers timeout_timers_;

  base::WeakPtrFactory<ReminderManager> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_REMINDER_REMINDER_MANAGER_H_
