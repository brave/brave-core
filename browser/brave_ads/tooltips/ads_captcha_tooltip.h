/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_TOOLTIPS_ADS_CAPTCHA_TOOLTIP_H_
#define BRAVE_BROWSER_BRAVE_ADS_TOOLTIPS_ADS_CAPTCHA_TOOLTIP_H_

#include <string>

#include "brave/browser/ui/brave_tooltips/brave_tooltip.h"
#include "brave/browser/ui/brave_tooltips/brave_tooltip_attributes.h"
#include "brave/components/brave_ads/browser/tooltips/ads_tooltips_delegate.h"

namespace brave_ads {

inline constexpr char kScheduledCaptchaTooltipId[] = "scheduled-captcha";

class AdsCaptchaTooltip : public brave_tooltips::BraveTooltip {
 public:
  AdsCaptchaTooltip(ShowScheduledCaptchaCallback show_captcha_callback,
                    SnoozeScheduledCaptchaCallback snooze_captcha_callback,
                    const brave_tooltips::BraveTooltipAttributes& attributes,
                    const std::string& payment_id,
                    const std::string& captcha_id);

  AdsCaptchaTooltip(const AdsCaptchaTooltip&) = delete;
  AdsCaptchaTooltip& operator=(const AdsCaptchaTooltip&) = delete;

  ~AdsCaptchaTooltip() override;

  const std::string& payment_id() const { return payment_id_; }
  const std::string& captcha_id() const { return captcha_id_; }

  // brave_tooltips::BraveTooltip:
  void PerformOkButtonAction() override;
  void PerformCancelButtonAction() override;

 private:
  ShowScheduledCaptchaCallback show_captcha_callback_;
  SnoozeScheduledCaptchaCallback snooze_captcha_callback_;
  std::string payment_id_;
  std::string captcha_id_;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_TOOLTIPS_ADS_CAPTCHA_TOOLTIP_H_
