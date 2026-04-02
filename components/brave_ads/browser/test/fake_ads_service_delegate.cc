/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/test/fake_ads_service_delegate.h"

#include "base/functional/callback.h"

namespace brave_ads::test {

FakeAdsServiceDelegate::FakeAdsServiceDelegate() = default;

FakeAdsServiceDelegate::~FakeAdsServiceDelegate() = default;

void FakeAdsServiceDelegate::MaybeInitNotificationHelper(
    base::OnceClosure /*callback*/) {}

bool FakeAdsServiceDelegate::
    CanShowSystemNotificationsWhileBrowserIsBackgrounded() {
  return false;
}

bool FakeAdsServiceDelegate::DoesSupportSystemNotifications() {
  return false;
}

bool FakeAdsServiceDelegate::CanShowNotifications() {
  return false;
}

bool FakeAdsServiceDelegate::ShowOnboardingNotification() {
  return false;
}

void FakeAdsServiceDelegate::ShowScheduledCaptcha(
    const std::string& /*payment_id*/,
    const std::string& /*captcha_id*/) {}

void FakeAdsServiceDelegate::ClearScheduledCaptcha() {}

void FakeAdsServiceDelegate::SnoozeScheduledCaptcha() {}

void FakeAdsServiceDelegate::ShowNotificationAd(const std::string& /*id*/,
                                                const std::u16string& /*title*/,
                                                const std::u16string& /*body*/,
                                                bool /*is_custom*/) {}

void FakeAdsServiceDelegate::CloseNotificationAd(const std::string& /*id*/,
                                                 bool /*is_custom*/) {}

void FakeAdsServiceDelegate::OpenNewTabWithUrl(const GURL& /*url*/) {}

bool FakeAdsServiceDelegate::IsFullScreenMode() {
  return false;
}

std::string FakeAdsServiceDelegate::GetVariationsCountryCode() {
  return "US";
}

}  // namespace brave_ads::test
