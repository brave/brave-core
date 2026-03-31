/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_ADS_ADS_SERVICE_DELEGATE_H_
#define BRAVE_BROWSER_BRAVE_ADS_ADS_SERVICE_DELEGATE_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "brave/components/brave_ads/core/browser/service/ads_service.h"

class GURL;
class NotificationDisplayService;
class PrefService;
class Profile;

namespace brave_adaptive_captcha {
class BraveAdaptiveCaptchaService;
}  // namespace brave_adaptive_captcha

namespace brave_ads {

// Singleton that owns all AdsService and associates them with Profiles.
class AdsServiceDelegate : public AdsService::Delegate {
 public:
  explicit AdsServiceDelegate(
      Profile& profile,
      PrefService* local_state,
      brave_adaptive_captcha::BraveAdaptiveCaptchaService&
          adaptive_captcha_service);

  AdsServiceDelegate(const AdsServiceDelegate&) = delete;
  AdsServiceDelegate& operator=(const AdsServiceDelegate&) = delete;

  ~AdsServiceDelegate() override;

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

 private:
  NotificationDisplayService* GetNotificationDisplayService();

  const raw_ref<Profile> profile_;
  const raw_ptr<PrefService> local_state_;  // Not owned.
  const raw_ref<brave_adaptive_captcha::BraveAdaptiveCaptchaService>
      adaptive_captcha_service_;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_BRAVE_ADS_ADS_SERVICE_DELEGATE_H_
