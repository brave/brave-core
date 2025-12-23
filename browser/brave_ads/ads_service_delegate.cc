/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/ads_service_delegate.h"

#include <utility>

#include "brave/browser/brave_ads/ad_units/notification_ad/notification_ad_platform_bridge.h"
#include "brave/browser/brave_ads/application_state/notification_helper/notification_helper.h"
#include "brave/browser/ui/brave_ads/notification_ad.h"
#include "brave/components/brave_adaptive_captcha/brave_adaptive_captcha_service.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/notifications/notification_display_service.h"
#include "chrome/browser/notifications/notification_display_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "components/variations/service/variations_service.h"
#include "components/variations/service/variations_service_utils.h"
#include "ui/message_center/public/cpp/notification.h"
#include "ui/message_center/public/cpp/notification_types.h"
#include "ui/message_center/public/cpp/notifier_id.h"
#include "url/gurl.h"

#if BUILDFLAG(IS_ANDROID)
#include "brave/browser/notifications/brave_notification_platform_bridge_helper_android.h"
#include "chrome/browser/android/service_tab_launcher.h"
#include "content/public/browser/page_navigator.h"
#else
#include "chrome/browser/fullscreen.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_navigator.h"
#endif

namespace brave_ads {

namespace {
constexpr char kNotificationAdUrlPrefix[] = "https://www.brave.com/ads/?";
}  // namespace

AdsServiceDelegate::AdsServiceDelegate(
    Profile& profile,
    PrefService* local_state,
    brave_adaptive_captcha::BraveAdaptiveCaptchaService&
        adaptive_captcha_service,
    std::unique_ptr<NotificationAdPlatformBridge>
        notification_ad_platform_bridge)
    : profile_(profile),
      local_state_(local_state),
      adaptive_captcha_service_(adaptive_captcha_service),
      notification_ad_platform_bridge_(
          std::move(notification_ad_platform_bridge)) {}

AdsServiceDelegate::~AdsServiceDelegate() {}

void AdsServiceDelegate::OpenNewTabWithUrl(const GURL& url) {
#if BUILDFLAG(IS_ANDROID)
  // ServiceTabLauncher can currently only launch new tabs
  const content::OpenURLParams params(url, content::Referrer(),
                                      WindowOpenDisposition::NEW_FOREGROUND_TAB,
                                      ui::PAGE_TRANSITION_LINK, true);
  ServiceTabLauncher::GetInstance()->LaunchTab(
      &*profile_, params, base::BindOnce([](content::WebContents*) {}));
#else
  Browser* browser = chrome::FindTabbedBrowser(&*profile_, false);
  if (!browser) {
    browser = Browser::Create(Browser::CreateParams(&*profile_, true));
  }
  NavigateParams nav_params(browser, url, ui::PAGE_TRANSITION_LINK);
  nav_params.disposition = WindowOpenDisposition::SINGLETON_TAB;
  nav_params.window_action = NavigateParams::WindowAction::kShowWindow;
  nav_params.path_behavior = NavigateParams::RESPECT;
  Navigate(&nav_params);
#endif
}

void AdsServiceDelegate::MaybeInitNotificationHelper(
    base::OnceClosure callback) {
  NotificationHelper::GetInstance()->MaybeInitForProfile(&*profile_,
                                                         std::move(callback));
}

bool AdsServiceDelegate::
    CanShowSystemNotificationsWhileBrowserIsBackgrounded() {
  return NotificationHelper::GetInstance()
      ->CanShowSystemNotificationsWhileBrowserIsBackgrounded();
}

bool AdsServiceDelegate::DoesSupportSystemNotifications() {
  return NotificationHelper::GetInstance()->DoesSupportSystemNotifications();
}

bool AdsServiceDelegate::CanShowNotifications() {
  return NotificationHelper::GetInstance()->CanShowNotifications();
}

bool AdsServiceDelegate::ShowOnboardingNotification() {
  return NotificationHelper::GetInstance()->ShowOnboardingNotification();
}

void AdsServiceDelegate::ShowScheduledCaptcha(const std::string& payment_id,
                                              const std::string& captcha_id) {
  adaptive_captcha_service_->ShowScheduledCaptcha(payment_id, captcha_id);
}

void AdsServiceDelegate::ClearScheduledCaptcha() {
  adaptive_captcha_service_->ClearScheduledCaptcha();
}

void AdsServiceDelegate::SnoozeScheduledCaptcha() {
  adaptive_captcha_service_->SnoozeScheduledCaptcha();
}

void AdsServiceDelegate::ShowNotificationAd(const std::string& id,
                                            const std::u16string& title,
                                            const std::u16string& body,
                                            bool is_custom) {
  if (is_custom) {
    notification_ad_platform_bridge_->ShowNotificationAd(
        NotificationAd(id, title, body, /*delegate=*/nullptr));
  } else {
    message_center::RichNotificationData notification_data;
    notification_data.context_message = u" ";

    const GURL url = GURL(kNotificationAdUrlPrefix + id);

    const std::unique_ptr<message_center::Notification> notification =
        std::make_unique<message_center::Notification>(
            message_center::NOTIFICATION_TYPE_SIMPLE, id, title, body,
            ui::ImageModel(), std::u16string(), url,
            message_center::NotifierId(
                message_center::NotifierType::SYSTEM_COMPONENT,
                "service.ads_service"),
            notification_data, nullptr);

#if !BUILDFLAG(IS_MAC) || defined(OFFICIAL_BUILD)
    // `set_never_timeout` uses an XPC service which requires signing so for now
    // we don't set this for macos dev builds
    notification->set_never_timeout(true);
#endif

    // We cannot store a raw_ptr to NotificationDisplayService due to upstream
    // browser tests changes NotificationDisplayService instance during test run
    // which leads to dangling pointer errors.
    GetNotificationDisplayService()->Display(
        NotificationHandler::Type::BRAVE_ADS, *notification, nullptr);
  }
}

void AdsServiceDelegate::CloseNotificationAd(const std::string& id,
                                             bool is_custom) {
  if (is_custom) {
    notification_ad_platform_bridge_->CloseNotificationAd(id);
  } else {
#if BUILDFLAG(IS_ANDROID)
    const std::string brave_ads_url_prefix = kNotificationAdUrlPrefix;
    const GURL url =
        GURL(brave_ads_url_prefix.substr(0, brave_ads_url_prefix.size() - 1));
    BraveNotificationPlatformBridgeHelperAndroid::MaybeRegenerateNotification(
        id, url);
#endif

    // We cannot store a raw_ptr to NotificationDisplayService due to upstream
    // browser tests changes NotificationDisplayService instance during test run
    // which leads to dangling pointer errors.
    GetNotificationDisplayService()->Close(NotificationHandler::Type::BRAVE_ADS,
                                           id);
  }
}

bool AdsServiceDelegate::IsFullScreenMode() {
#if !BUILDFLAG(IS_ANDROID)
  return ::IsFullScreenMode();
#else
  return true;
#endif
}

std::string AdsServiceDelegate::GetVariationsCountryCode() {
  std::string country_code;

  variations::VariationsService* variations_service =
      g_browser_process->variations_service();

  if (variations_service) {
    country_code = variations_service->GetLatestCountry();
  }

  if (country_code.empty()) {
    // May be empty on first run after a fresh install, so fall back to the
    // permanently stored variations or device country code on first run.
    country_code = variations::GetCurrentCountryCode(variations_service);
  }

  // Convert the country code to an ISO 3166-1 alpha-2 format. This ensures the
  // country code is in uppercase, as required by the standard.
  return base::ToUpperASCII(country_code);
}

NotificationDisplayService*
AdsServiceDelegate::GetNotificationDisplayService() {
  return NotificationDisplayServiceFactory::GetForProfile(&*profile_);
}

}  // namespace brave_ads
