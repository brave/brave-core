/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_OBSERVER_INTERFACE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_OBSERVER_INTERFACE_H_

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"

namespace brave_ads {

class AdsObserverInterface {
 public:
  virtual ~AdsObserverInterface() = default;

  // Invoked when ad rewards have changed.
  virtual void OnAdRewardsDidChange() = 0;

  // Invoked when a browser upgrade is required to serve ads.
  virtual void OnBrowserUpgradeRequiredToServeAds() = 0;

  // Invoked when a Rewards wallet is deemed ineligible to serve ads.
  virtual void OnIneligibleRewardsWalletToServeAds() = 0;

  // Invoked to remind the user of what to do and what not to do.
  virtual void OnRemindUser(mojom::ReminderType mojom_reminder_type) = 0;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_OBSERVER_INTERFACE_H_
