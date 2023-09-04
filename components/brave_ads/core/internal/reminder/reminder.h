/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_REMINDER_REMINDER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_REMINDER_REMINDER_H_

#include "base/timer/timer.h"
#include "brave/components/brave_ads/core/internal/history/history_manager_observer.h"

namespace brave_ads {

struct HistoryItemInfo;

class Reminder : public HistoryManagerObserver {
 public:
  Reminder();

  Reminder(const Reminder&) = delete;
  Reminder& operator=(const Reminder&) = delete;

  Reminder(Reminder&&) noexcept = delete;
  Reminder& operator=(Reminder&&) noexcept = delete;

  ~Reminder() override;

 private:
  // HistoryManagerObserver:
  void OnDidAddHistory(const HistoryItemInfo& history_item) override;

  base::OneShotTimer timer_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_REMINDER_REMINDER_H_
