/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_OBSERVER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_OBSERVER_H_

#include <string>

#include "brave/components/brave_ads/buildflags/buildflags.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

static_assert(BUILDFLAG(ENABLE_BRAVE_ADS));

namespace brave_ads {

class AdsObserver {
 public:
  virtual ~AdsObserver() = default;

  // Invoked when ad rewards have changed.
  virtual void OnAdRewardsDidChange() {}

  // Invoked when a browser upgrade is required to serve ads.
  virtual void OnBrowserUpgradeRequiredToServeAds() {}

  // Invoked when a wallet is deemed ineligible to serve ads.
  virtual void OnIneligibleWalletToServeAds() {}

  // Invoked when the user must solve a captcha to continue to be served ads.
  virtual void OnSolveCaptchaToServeAds(const std::string& payment_id,
                                        const std::string& captcha_id) = 0;

  // Invoked to remind the user of what to do and what not to do.
  virtual void OnRemindUser(mojom::ReminderType mojom_reminder_type) {}
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_OBSERVER_H_
