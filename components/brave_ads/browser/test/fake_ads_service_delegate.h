/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_TEST_FAKE_ADS_SERVICE_DELEGATE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_TEST_FAKE_ADS_SERVICE_DELEGATE_H_

#include <string>

#include "brave/components/brave_ads/core/browser/service/ads_service.h"

class GURL;

namespace brave_ads::test {

// Minimal no-op implementation of `AdsService::Delegate` for use in unit
// tests that exercise `AdsServiceImpl` without a real browser environment.
class FakeAdsServiceDelegate : public AdsService::Delegate {
 public:
  FakeAdsServiceDelegate();

  FakeAdsServiceDelegate(const FakeAdsServiceDelegate&) = delete;
  FakeAdsServiceDelegate& operator=(const FakeAdsServiceDelegate&) = delete;

  ~FakeAdsServiceDelegate() override;

  // AdsService::Delegate:
  void MaybeInitNotificationHelper(base::OnceClosure callback) override;
  bool CanShowSystemNotificationsWhileBrowserIsBackgrounded() override;
  bool DoesSupportSystemNotifications() override;
  bool CanShowNotifications() override;
  bool ShowOnboardingNotification() override;
  void ShowScheduledCaptcha(const std::string& payment_id,
                            const std::string& captcha_id) override;
  void ClearScheduledCaptcha() override;
  void SnoozeScheduledCaptcha() override;
  void ShowNotificationAd(const std::string& id,
                          const std::u16string& title,
                          const std::u16string& body,
                          bool is_custom) override;
  void CloseNotificationAd(const std::string& id, bool is_custom) override;
  void OpenNewTabWithUrl(const GURL& url) override;
  bool IsFullScreenMode() override;
  std::string GetVariationsCountryCode() override;
};

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_TEST_FAKE_ADS_SERVICE_DELEGATE_H_
