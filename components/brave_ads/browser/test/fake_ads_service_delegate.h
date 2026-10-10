/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_TEST_FAKE_ADS_SERVICE_DELEGATE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_TEST_FAKE_ADS_SERVICE_DELEGATE_H_

#include <optional>
#include <string>
#include <vector>

#include "brave/components/brave_ads/core/browser/service/ads_service.h"
#include "url/gurl.h"

namespace brave_ads::test {

// Minimal no-op implementation of `AdsService::Delegate` for use in unit
// tests that exercise `AdsServiceImpl` without a real browser environment.
// `CanShowNotifications()` returns `false` and all other methods are no-ops
// unless configured otherwise; notification/tab actions are recorded so
// tests can assert on them.
class FakeAdsServiceDelegate : public AdsService::Delegate {
 public:
  FakeAdsServiceDelegate();

  FakeAdsServiceDelegate(const FakeAdsServiceDelegate&) = delete;
  FakeAdsServiceDelegate& operator=(const FakeAdsServiceDelegate&) = delete;

  ~FakeAdsServiceDelegate() override;

  // AdsService::Delegate:
  void MaybeInitNotificationHelper() override;
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
                          const std::u16string& body) override;
  void CloseNotificationAd(const std::string& id) override;
  void OpenNewTabWithUrl(const GURL& url) override;
  bool IsFullScreenMode() override;
  std::string GetVariationsCountryCode() override;

  void set_can_show_notifications(bool can_show_notifications) {
    can_show_notifications_ = can_show_notifications;
  }

  const std::optional<std::string>& last_shown_notification_ad_placement_id()
      const {
    return last_shown_notification_ad_placement_id_;
  }

  const std::optional<std::u16string>& last_shown_notification_ad_title()
      const {
    return last_shown_notification_ad_title_;
  }

  const std::optional<std::u16string>& last_shown_notification_ad_body() const {
    return last_shown_notification_ad_body_;
  }

  const std::vector<std::string>& closed_notification_ad_ids() const {
    return closed_notification_ad_ids_;
  }

  const std::optional<GURL>& last_opened_url() const {
    return last_opened_url_;
  }

 private:
  bool can_show_notifications_ = false;

  std::optional<std::string> last_shown_notification_ad_placement_id_;
  std::optional<std::u16string> last_shown_notification_ad_title_;
  std::optional<std::u16string> last_shown_notification_ad_body_;
  std::vector<std::string> closed_notification_ad_ids_;
  std::optional<GURL> last_opened_url_;
};

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_TEST_FAKE_ADS_SERVICE_DELEGATE_H_
