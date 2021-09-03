/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/tooltips/ads_captcha_tooltip.h"

#include <utility>

namespace brave_ads {

const char kScheduledCaptchaTooltipId[] = "scheduled-captcha";

AdsCaptchaTooltip::AdsCaptchaTooltip(
    ShowScheduledCaptchaCallback show_captcha_callback,
    SnoozeScheduledCaptchaCallback snooze_captcha_callback,
    const brave_tooltips::BraveTooltipAttributes& attributes,
    const std::string& payment_id,
    const std::string& captcha_id)
    : BraveTooltip(kScheduledCaptchaTooltipId, attributes, nullptr),
      show_captcha_callback_(std::move(show_captcha_callback)),
      snooze_captcha_callback_(std::move(snooze_captcha_callback)),
      payment_id_(payment_id),
      captcha_id_(captcha_id) {}

AdsCaptchaTooltip::~AdsCaptchaTooltip() = default;

void AdsCaptchaTooltip::PerformOkButtonAction() {
  // User chose to solve the captcha now, so show it to initiate that process
  if (show_captcha_callback_) {
    std::move(show_captcha_callback_).Run(payment_id_, captcha_id_);
  }
}

void AdsCaptchaTooltip::PerformCancelButtonAction() {
  // In this context, cancel means snooze the captcha for now
  if (snooze_captcha_callback_) {
    std::move(snooze_captcha_callback_).Run();
  }
}

}  // namespace brave_ads
