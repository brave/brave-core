/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_OBSERVER_H_
#define BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_OBSERVER_H_

#include "brave/components/brave_ads/core/public/ads_observer_interface.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace bat_ads {

class BatAdsObserver : public brave_ads::AdsObserverInterface {
 public:
  BatAdsObserver();
  explicit BatAdsObserver(mojo::PendingRemote<mojom::BatAdsObserver>
                              bat_ads_observer_pending_remote);

  BatAdsObserver(const BatAdsObserver&);
  BatAdsObserver& operator=(const BatAdsObserver&);

  BatAdsObserver(BatAdsObserver&&) noexcept;
  BatAdsObserver& operator=(BatAdsObserver&&) noexcept;

  ~BatAdsObserver() override;

  void OnAdRewardsDidChange() override;

  void OnBrowserUpgradeRequiredToServeAds() override;

  void OnIneligibleRewardsWalletToServeAds() override;

  void OnRemindUser(
      brave_ads::mojom::ReminderType mojom_reminder_type) override;

 private:
  mojo::Remote<mojom::BatAdsObserver> bat_ads_observer_remote_;
};

}  // namespace bat_ads

#endif  // BRAVE_COMPONENTS_SERVICES_BAT_ADS_BAT_ADS_OBSERVER_H_
