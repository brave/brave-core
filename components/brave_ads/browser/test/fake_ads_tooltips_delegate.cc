/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/test/fake_ads_tooltips_delegate.h"

namespace brave_ads::test {

FakeAdsTooltipsDelegate::FakeAdsTooltipsDelegate() = default;

FakeAdsTooltipsDelegate::~FakeAdsTooltipsDelegate() = default;

void FakeAdsTooltipsDelegate::ShowCaptchaTooltip(
    const std::string& /*payment_id*/,
    const std::string& /*captcha_id*/,
    bool /*include_cancel_button*/,
    ShowScheduledCaptchaCallback /*show_captcha_callback*/,
    SnoozeScheduledCaptchaCallback /*snooze_captcha_callback*/) {}

void FakeAdsTooltipsDelegate::CloseCaptchaTooltip() {}

}  // namespace brave_ads::test
