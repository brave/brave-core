/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_TOOLTIPS_DELEGATE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_TOOLTIPS_DELEGATE_H_

#include <string>

namespace brave_ads {

using ShowScheduledCaptchaCallback =
    base::OnceCallback<void(const std::string&, const std::string&)>;
using SnoozeScheduledCaptchaCallback = base::OnceCallback<void()>;

class AdsTooltipsDelegate {
 public:
  virtual ~AdsTooltipsDelegate() = default;

  // Called to show the captcha tooltip
  virtual void ShowCaptchaTooltip(
      const std::string& payment_id,
      const std::string& captcha_id,
      bool enable_cancel_button,
      ShowScheduledCaptchaCallback show_captcha_callback,
      SnoozeScheduledCaptchaCallback snooze_captcha_callback) = 0;

  // Called to close the captcha tooltip
  virtual void CloseCaptchaTooltip() = 0;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_TOOLTIPS_DELEGATE_H_
