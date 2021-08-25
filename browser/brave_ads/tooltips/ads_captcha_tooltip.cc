/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/tooltips/ads_captcha_tooltip.h"

#include "brave/components/brave_adaptive_captcha/buildflags/buildflags.h"

#if BUILDFLAG(BRAVE_ADAPTIVE_CAPTCHA_ENABLED)
#include "brave/components/brave_adaptive_captcha/brave_adaptive_captcha_service.h"
#endif

namespace brave_ads {

const char kScheduledCaptchaTooltipId[] = "scheduled-captcha";

AdsCaptchaTooltip::AdsCaptchaTooltip(
    brave_adaptive_captcha::BraveAdaptiveCaptchaService*
        adaptive_captcha_service,
    const brave_tooltips::BraveTooltipAttributes& attributes,
    const std::string& payment_id,
    const std::string& captcha_id)
    : BraveTooltip(kScheduledCaptchaTooltipId, attributes, nullptr),
      adaptive_captcha_service_(adaptive_captcha_service),
      payment_id_(payment_id),
      captcha_id_(captcha_id) {}

AdsCaptchaTooltip::~AdsCaptchaTooltip() = default;

void AdsCaptchaTooltip::PerformOkButtonAction() {
  // User chose to solve the captcha now, so show it to initiate that process
  if (adaptive_captcha_service_) {
    adaptive_captcha_service_->ShowScheduledCaptcha(payment_id_, captcha_id_);
  }
}

void AdsCaptchaTooltip::PerformCancelButtonAction() {
  // In this context, cancel means snooze the captcha for now
  if (adaptive_captcha_service_) {
    adaptive_captcha_service_->SnoozeScheduledCaptcha();
  }
}

}  // namespace brave_ads
