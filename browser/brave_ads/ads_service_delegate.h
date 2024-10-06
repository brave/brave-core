/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_ADS_SERVICE_DELEGATE_H_
#define BRAVE_BROWSER_BRAVE_ADS_ADS_SERVICE_DELEGATE_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_ads/browser/ads_service.h"

class Profile;

namespace brave_adaptive_captcha {
class BraveAdaptiveCaptchaService;
}

class NotificationDisplayService;

namespace brave_ads {

class NotificationAdPlatformBridge;

// Singleton that owns all AdsService and associates them with Profiles.
class AdsServiceDelegate : public AdsService::Delegate {
 public:
  explicit AdsServiceDelegate(
      Profile* profile,
      brave_adaptive_captcha::BraveAdaptiveCaptchaService*
          adaptive_captcha_service,
      NotificationDisplayService* notification_display_service,
      std::unique_ptr<NotificationAdPlatformBridge>
          notification_ad_platform_bridge);
  ~AdsServiceDelegate() override;
  AdsServiceDelegate(const AdsServiceDelegate&) = delete;
  AdsServiceDelegate& operator=(const AdsServiceDelegate&) = delete;

  AdsServiceDelegate(AdsServiceDelegate&&) noexcept = delete;
  AdsServiceDelegate& operator=(AdsServiceDelegate&&) noexcept = delete;

  // AdsService::Delegate implementation
  void InitNotificationHelper() override;
  bool CanShowSystemNotificationsWhileBrowserIsBackgrounded() override;
  bool DoesSupportSystemNotifications() override;
  bool CanShowNotifications() override;
  bool ShowOnboardingNotification() override;
  void ShowScheduledCaptcha(const std::string& payment_id,
                            const std::string& captcha_id) override;
  void ClearScheduledCaptcha() override;
  void SnoozeScheduledCaptcha() override;
  void Display(const message_center::Notification& notification) override;
  void Close(const std::string& notification_id) override;
  void ShowNotificationAd(const std::string& id,
                          const std::u16string& title,
                          const std::u16string& body) override;
  void CloseNotificationAd(const std::string& id) override;
  void OpenNewTabWithUrl(const GURL& url) override;
#if BUILDFLAG(IS_ANDROID)
  void MaybeRegenerateNotification(const std::string& notification_id,
                                   const GURL& service_worker_scope) override;
#else
  bool IsFullScreenMode() override;
#endif
 private:
  raw_ptr<Profile> profile_;
  raw_ptr<brave_adaptive_captcha::BraveAdaptiveCaptchaService>
      adaptive_captcha_service_;
  raw_ptr<NotificationDisplayService> notification_display_service_;
  std::unique_ptr<NotificationAdPlatformBridge>
      notification_ad_platform_bridge_;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_ADS_SERVICE_DELEGATE_H_
