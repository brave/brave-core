/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_TEST_FAKE_ADS_TOOLTIPS_DELEGATE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_TEST_FAKE_ADS_TOOLTIPS_DELEGATE_H_

#include <string>

#include "brave/components/brave_ads/browser/tooltips/ads_tooltips_delegate.h"

namespace brave_ads::test {

// No-op implementation of `AdsTooltipsDelegate` for unit tests.
class FakeAdsTooltipsDelegate : public AdsTooltipsDelegate {
 public:
  FakeAdsTooltipsDelegate();

  FakeAdsTooltipsDelegate(const FakeAdsTooltipsDelegate&) = delete;
  FakeAdsTooltipsDelegate& operator=(const FakeAdsTooltipsDelegate&) = delete;

  ~FakeAdsTooltipsDelegate() override;

  // AdsTooltipsDelegate:
  void ShowCaptchaTooltip(
      const std::string& payment_id,
      const std::string& captcha_id,
      bool include_cancel_button,
      ShowScheduledCaptchaCallback show_captcha_callback,
      SnoozeScheduledCaptchaCallback snooze_captcha_callback) override;
  void CloseCaptchaTooltip() override;
};

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_TEST_FAKE_ADS_TOOLTIPS_DELEGATE_H_
