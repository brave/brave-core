/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/ads_service_delegate.h"

#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "base/version_info/channel.h"
#include "base/version_info/version_info.h"
#include "brave/browser/brave_ads/ad_units/notification_ad/notification_ad_platform_bridge.h"
#include "brave/browser/brave_ads/application_state/notification_helper/notification_helper.h"
#include "brave/browser/ui/brave_ads/notification_ad.h"
#include "brave/components/brave_adaptive_captcha/brave_adaptive_captcha_service.h"
#include "build/build_config.h"
#include "chrome/browser/notifications/notification_display_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/channel_info.h"
#include "components/search_engines/template_url_prepopulate_data.h"

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

AdsServiceDelegate::AdsServiceDelegate(
    Profile* profile,
    PrefService* local_state,
    brave_adaptive_captcha::BraveAdaptiveCaptchaService*
        adaptive_captcha_service,
    NotificationDisplayService* notification_display_service,
    std::unique_ptr<NotificationAdPlatformBridge>
        notification_ad_platform_bridge)
    : profile_(profile),
      local_state_(local_state),
      search_engine_choice_service_(
          *profile_->GetPrefs(),
          local_state_,
          /*is_profile_eligible_for_dse_guest_propagation=*/false),
      adaptive_captcha_service_(adaptive_captcha_service),
      notification_display_service_(notification_display_service),
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
      profile_, params, base::BindOnce([](content::WebContents*) {}));
#else
  Browser* browser = chrome::FindTabbedBrowser(profile_, false);
  if (!browser) {
    browser = Browser::Create(Browser::CreateParams(profile_, true));
  }
  NavigateParams nav_params(browser, url, ui::PAGE_TRANSITION_LINK);
  nav_params.disposition = WindowOpenDisposition::SINGLETON_TAB;
  nav_params.window_action = NavigateParams::SHOW_WINDOW;
  nav_params.path_behavior = NavigateParams::RESPECT;
  Navigate(&nav_params);
#endif
}

void AdsServiceDelegate::InitNotificationHelper() {
  NotificationHelper::GetInstance()->InitForProfile(profile_);
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

void AdsServiceDelegate::Display(
    const message_center::Notification& notification) {
  notification_display_service_->Display(NotificationHandler::Type::BRAVE_ADS,
                                         notification, nullptr);
}

void AdsServiceDelegate::Close(const std::string& notification_id) {
  notification_display_service_->Close(NotificationHandler::Type::BRAVE_ADS,
                                       notification_id);
}

void AdsServiceDelegate::ShowNotificationAd(const std::string& id,
                                            const std::u16string& title,
                                            const std::u16string& body) {
  notification_ad_platform_bridge_->ShowNotificationAd(
      NotificationAd(id, title, body, nullptr));
}

void AdsServiceDelegate::CloseNotificationAd(const std::string& id) {
  notification_ad_platform_bridge_->CloseNotificationAd(id);
}

#if BUILDFLAG(IS_ANDROID)
void AdsServiceDelegate::MaybeRegenerateNotification(
    const std::string& placement_id,
    const GURL& url) {
  BraveNotificationPlatformBridgeHelperAndroid::MaybeRegenerateNotification(
      placement_id, url);
}
#else
bool AdsServiceDelegate::IsFullScreenMode() {
  return ::IsFullScreenMode();
}
#endif

base::Value::Dict AdsServiceDelegate::GetVirtualPrefs() {
  const auto template_url_data =
      TemplateURLPrepopulateData::GetPrepopulatedFallbackSearch(
          profile_->GetPrefs(), &search_engine_choice_service_);
  if (!template_url_data) {
    return {};
  }

  return base::Value::Dict()
      .Set("[virtual]:build_channel.name",
           version_info::GetChannelString(chrome::GetChannel()))
      .Set("[virtual]:default_search_engine.name",
           base::UTF16ToUTF8(template_url_data->short_name()));
}

}  // namespace brave_ads
