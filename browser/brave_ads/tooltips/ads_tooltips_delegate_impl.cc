/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/tooltips/ads_tooltips_delegate_impl.h"

#include <utility>

namespace brave_ads {

AdsTooltipsDelegateImpl::AdsTooltipsDelegateImpl() {}

void AdsTooltipsDelegateImpl::ShowCaptchaTooltip(
    const std::string& payment_id,
    const std::string& captcha_id,
    bool include_cancel_button,
    ShowScheduledCaptchaCallback show_captcha_callback,
    SnoozeScheduledCaptchaCallback snooze_captcha_callback) {
  ads_tooltips_controller_.ShowCaptchaTooltip(
      payment_id, captcha_id, include_cancel_button,
      std::move(show_captcha_callback), std::move(snooze_captcha_callback));
}

void AdsTooltipsDelegateImpl::CloseCaptchaTooltip() {
  ads_tooltips_controller_.CloseCaptchaTooltip();
}

}  // namespace brave_ads
