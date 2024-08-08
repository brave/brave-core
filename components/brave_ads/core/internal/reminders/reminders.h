/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_REMINDERS_REMINDERS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_REMINDERS_REMINDERS_H_

#include "base/timer/timer.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_manager_observer.h"

namespace brave_ads {

struct AdHistoryItemInfo;

class Reminders : public AdHistoryManagerObserver {
 public:
  Reminders();

  Reminders(const Reminders&) = delete;
  Reminders& operator=(const Reminders&) = delete;

  Reminders(Reminders&&) noexcept = delete;
  Reminders& operator=(Reminders&&) noexcept = delete;

  ~Reminders() override;

 private:
  void MaybeShowReminderAfterDelay(const AdHistoryItemInfo& ad_history_item);

  // AdHistoryManagerObserver:
  void OnDidAppendAdHistoryItem(
      const AdHistoryItemInfo& ad_history_item) override;

  base::OneShotTimer timer_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_REMINDERS_REMINDERS_H_
