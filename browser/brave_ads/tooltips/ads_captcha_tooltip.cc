/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/tooltips/ads_captcha_tooltip.h"

#include "brave/browser/brave_ads/ads_service_factory.h"
#include "chrome/browser/profiles/profile.h"

namespace brave_ads {

const char kScheduledCaptchaTooltipId[] = "scheduled-captcha";

AdsCaptchaTooltip::AdsCaptchaTooltip(
    Profile* profile,
    const brave_tooltips::BraveTooltipAttributes& attributes,
    const std::string& payment_id,
    const std::string& captcha_id)
    : BraveTooltip(kScheduledCaptchaTooltipId, attributes, nullptr),
      profile_(profile),
      payment_id_(payment_id),
      captcha_id_(captcha_id) {}

AdsCaptchaTooltip::~AdsCaptchaTooltip() = default;

void AdsCaptchaTooltip::PerformOkButtonAction() {
  // User chose to solve the captcha now, so show it to initiate that process
  if (auto* ads_service = AdsServiceFactory::GetForProfile(profile_)) {
    ads_service->ShowScheduledCaptcha(payment_id_, captcha_id_);
  }
}

void AdsCaptchaTooltip::PerformCancelButtonAction() {
  // In this context, cancel means snooze the captcha for now
  if (auto* ads_service = AdsServiceFactory::GetForProfile(profile_)) {
    ads_service->SnoozeScheduledCaptcha();
  }
}

}  // namespace brave_ads
