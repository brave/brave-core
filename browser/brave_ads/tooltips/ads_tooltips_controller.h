/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_TOOLTIPS_ADS_TOOLTIPS_CONTROLLER_H_
#define BRAVE_BROWSER_BRAVE_ADS_TOOLTIPS_ADS_TOOLTIPS_CONTROLLER_H_

#include <string>

#include "brave/browser/ui/brave_tooltips/brave_tooltip_delegate.h"
#include "brave/components/brave_ads/browser/tooltips/ads_tooltips_delegate.h"

namespace brave_ads {

class AdsTooltipsController final
    : public AdsTooltipsDelegate,
      public brave_tooltips::BraveTooltipDelegate {
 public:
  AdsTooltipsController();

  AdsTooltipsController(const AdsTooltipsController&) = delete;
  AdsTooltipsController& operator=(const AdsTooltipsController&) = delete;

  AdsTooltipsController(AdsTooltipsController&&) noexcept = delete;
  AdsTooltipsController& operator=(AdsTooltipsController&&) noexcept = delete;

  ~AdsTooltipsController() override;

  // AdsTooltipDelegate:
  void ShowCaptchaTooltip(
      const std::string& payment_id,
      const std::string& captcha_id,
      bool include_cancel_button,
      ShowScheduledCaptchaCallback show_captcha_callback,
      SnoozeScheduledCaptchaCallback snooze_captcha_callback) override;
  void CloseCaptchaTooltip() override;

 private:
  // brave_tooltips::BraveTooltipDelegate:
  void OnTooltipWidgetDestroyed(const std::string& tooltip_id) override;

  base::WeakPtr<brave_tooltips::BraveTooltipDelegate> AsWeakPtr() override;

  base::WeakPtrFactory<BraveTooltipDelegate> weak_ptr_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_TOOLTIPS_ADS_TOOLTIPS_CONTROLLER_H_
