/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_TOOLTIPS_ADS_TOOLTIPS_CONTROLLER_H_
#define BRAVE_BROWSER_BRAVE_ADS_TOOLTIPS_ADS_TOOLTIPS_CONTROLLER_H_

#include <map>
#include <string>

#include "brave/browser/brave_ads/tooltips/ads_captcha_tooltip.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/ui/brave_tooltips/brave_tooltip.h"
#include "brave/components/brave_ads/browser/ads_tooltips_delegate.h"

namespace brave_tooltips {
class BraveTooltipPopup;
}  // namespace brave_tooltips

class Profile;

namespace brave_ads {

class AdsTooltipsController : public AdsTooltipsDelegate,
                              public brave_tooltips::BraveTooltipDelegate {
 public:
  explicit AdsTooltipsController(Profile* profile);
  ~AdsTooltipsController() override;

  AdsTooltipsController(const AdsTooltipsController&) = delete;
  AdsTooltipsController& operator=(const AdsTooltipsController&) = delete;

  // AdsTooltipDelegate:
  void ShowCaptchaTooltip(
      const std::string& payment_id,
      const std::string& captcha_id,
      bool enable_cancel_button,
      ShowScheduledCaptchaCallback show_captcha_callback,
      SnoozeScheduledCaptchaCallback snooze_captcha_callback) override;
  void CloseCaptchaTooltip() override;

 private:
  // brave_tooltips::BraveTooltipDelegate:
  void OnTooltipWidgetDestroyed(const std::string& tooltip_id) override;

  Profile* profile_ = nullptr;  // NOT OWNED
  std::map<std::string, brave_tooltips::BraveTooltipPopup* /* NOT OWNED */>
      tooltip_popups_;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_TOOLTIPS_ADS_TOOLTIPS_CONTROLLER_H_
