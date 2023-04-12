/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_REMINDER_REMINDER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_REMINDER_REMINDER_H_

#include "brave/components/brave_ads/core/internal/history/history_manager_observer.h"

namespace brave_ads {

struct HistoryItemInfo;

class Reminder : public HistoryManagerObserver {
 public:
  Reminder();

  Reminder(const Reminder& other) = delete;
  Reminder& operator=(const Reminder& other) = delete;

  Reminder(Reminder&& other) noexcept = delete;
  Reminder& operator=(Reminder&& other) noexcept = delete;

  ~Reminder() override;

 private:
  // HistoryManagerObserver:
  void OnDidAddHistory(const HistoryItemInfo& history_item) override;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_REMINDER_REMINDER_H_
