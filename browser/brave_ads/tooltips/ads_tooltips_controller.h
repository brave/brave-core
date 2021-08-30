/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_TOOLTIPS_ADS_TOOLTIPS_CONTROLLER_H_
#define BRAVE_BROWSER_BRAVE_ADS_TOOLTIPS_ADS_TOOLTIPS_CONTROLLER_H_

#include <map>
#include <string>

#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/ui/brave_tooltips/brave_tooltip.h"
#include "brave/components/brave_adaptive_captcha/buildflags/buildflags.h"
#include "brave/components/brave_ads/browser/ads_tooltips_delegate.h"

namespace brave_adaptive_captcha {
class BraveAdaptiveCaptchaService;
}  // namespace brave_adaptive_captcha

namespace brave_tooltips {
class BraveTooltipPopup;
}  // namespace brave_tooltips

class Profile;

namespace brave_ads {

class AdsTooltipsController : public AdsTooltipsDelegate,
                              public brave_tooltips::BraveTooltipDelegate {
 public:
  explicit AdsTooltipsController(
#if BUILDFLAG(BRAVE_ADAPTIVE_CAPTCHA_ENABLED)
      brave_adaptive_captcha::BraveAdaptiveCaptchaService*
          brave_adaptive_captcha_service,
#endif
      Profile* profile);
  ~AdsTooltipsController() override;

  AdsTooltipsController(const AdsTooltipsController&) = delete;
  AdsTooltipsController& operator=(const AdsTooltipsController&) = delete;

  // AdsTooltipDelegate:
#if BUILDFLAG(BRAVE_ADAPTIVE_CAPTCHA_ENABLED)
  void ShowCaptchaTooltip(const std::string& payment_id,
                          const std::string& captcha_id,
                          bool enable_cancel_button) override;
  void CloseCaptchaTooltip() override;
#endif

 private:
  // brave_tooltips::BraveTooltipDelegate:
  void OnTooltipWidgetDestroyed(const std::string& tooltip_id) override;

  brave_adaptive_captcha::BraveAdaptiveCaptchaService*
      brave_adaptive_captcha_service_ = nullptr;  // NOT OWNED
  Profile* profile_ = nullptr;                    // NOT OWNED
  std::map<std::string, brave_tooltips::BraveTooltipPopup* /* NOT OWNED */>
      tooltip_popups_;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_TOOLTIPS_ADS_TOOLTIPS_CONTROLLER_H_
