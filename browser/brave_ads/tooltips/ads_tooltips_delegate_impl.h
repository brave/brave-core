/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_TOOLTIPS_ADS_TOOLTIPS_DELEGATE_IMPL_H_
#define BRAVE_BROWSER_BRAVE_ADS_TOOLTIPS_ADS_TOOLTIPS_DELEGATE_IMPL_H_

#include <string>

#include "brave/browser/brave_ads/tooltips/ads_tooltips_controller.h"
#include "brave/components/brave_ads/browser/ads_tooltips_delegate.h"

class Profile;

namespace brave_ads {

class AdsTooltipsDelegateImpl : public AdsTooltipsDelegate {
 public:
  explicit AdsTooltipsDelegateImpl(Profile* profile);
  ~AdsTooltipsDelegateImpl() override = default;

  AdsTooltipsDelegateImpl(const AdsTooltipsDelegateImpl&) = delete;
  AdsTooltipsDelegateImpl& operator=(const AdsTooltipsDelegateImpl&) = delete;

  void ShowCaptchaTooltip(
      const std::string& payment_id,
      const std::string& captcha_id,
      bool enable_cancel_button,
      ShowScheduledCaptchaCallback show_captcha_callback,
      SnoozeScheduledCaptchaCallback snooze_captcha_callback) override;
  void CloseCaptchaTooltip() override;

 private:
  AdsTooltipsController ads_tooltips_controller_;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_TOOLTIPS_ADS_TOOLTIPS_DELEGATE_IMPL_H_
