/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_NOTIFIER_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_NOTIFIER_MANAGER_H_

#include <memory>
#include <vector>

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"

namespace brave_ads {

class AdsObserverInterface;

class AdsNotifierManager final {
 public:
  AdsNotifierManager();

  AdsNotifierManager(const AdsNotifierManager& other) = delete;
  AdsNotifierManager& operator=(const AdsNotifierManager& other) = delete;

  AdsNotifierManager(AdsNotifierManager&& other) noexcept = delete;
  AdsNotifierManager& operator=(AdsNotifierManager&& other) noexcept = delete;

  ~AdsNotifierManager();

  static AdsNotifierManager& GetInstance();

  void AddObserver(std::unique_ptr<AdsObserverInterface> observer);

  void NotifyAdRewardsDidChange() const;

  void NotifyBrowserUpgradeRequiredToServeAds() const;

  void NotifyIneligibleRewardsWalletToServeAds() const;

  void NotifyRemindUser(mojom::ReminderType mojom_reminder_type) const;

 private:
  std::vector<std::unique_ptr<AdsObserverInterface>> observers_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_NOTIFIER_MANAGER_H_
