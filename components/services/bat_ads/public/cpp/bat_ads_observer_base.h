/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BAT_ADS_PUBLIC_CPP_BAT_ADS_OBSERVER_BASE_H_
#define BRAVE_COMPONENTS_SERVICES_BAT_ADS_PUBLIC_CPP_BAT_ADS_OBSERVER_BASE_H_

// Mojo generated interfaces require every method to be implemented, even when
// most events are unused. Browser observers typically only care about a few ad
// events, so empty overrides add unnecessary noise. This base class provides
// no-op defaults for all `mojom::BatAdsObserver` methods, allowing subclasses
// to override only what they need.

#include <string>

#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"

namespace bat_ads {

class BatAdsObserverBase : public mojom::BatAdsObserver {
 public:
  void OnAdRewardsDidChange() override {}
  void OnBrowserUpgradeRequiredToServeAds() override {}
  void OnIneligibleWalletToServeAds() override {}
  void OnSolveCaptchaToServeAds(const std::string& /*payment_id*/,
                                const std::string& /*captcha_id*/) override {}
  void OnRemindUser(
      brave_ads::mojom::ReminderType /*mojom_reminder_type*/) override {}
};

}  // namespace bat_ads

#endif  // BRAVE_COMPONENTS_SERVICES_BAT_ADS_PUBLIC_CPP_BAT_ADS_OBSERVER_BASE_H_
