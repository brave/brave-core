/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_BRAVE_ADS_ADS_CLIENT_IOS_OBSERVER_H_
#define BRAVE_IOS_BROWSER_BRAVE_ADS_ADS_CLIENT_IOS_OBSERVER_H_

// Forwards ads events to the iOS client delegate, since iOS does not use the
// inter-process messaging channel available on other platforms.

#include <string>

#include "base/functional/callback.h"
#include "brave/components/brave_ads/core/public/ads_observer.h"

namespace brave_ads {

using SolveCaptchaCallback =
    base::RepeatingCallback<void(const std::string&, const std::string&)>;

class AdsClientIOSObserver : public AdsObserver {
 public:
  explicit AdsClientIOSObserver(SolveCaptchaCallback callback);

  AdsClientIOSObserver(const AdsClientIOSObserver&) = delete;
  AdsClientIOSObserver& operator=(const AdsClientIOSObserver&) = delete;

  ~AdsClientIOSObserver() override;

  void OnSolveCaptchaToServeAds(const std::string& payment_id,
                                const std::string& captcha_id) override;

 private:
  SolveCaptchaCallback callback_;
};

}  // namespace brave_ads

#endif  // BRAVE_IOS_BROWSER_BRAVE_ADS_ADS_CLIENT_IOS_OBSERVER_H_
