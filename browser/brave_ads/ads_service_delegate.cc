/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/ads_service_delegate.h"

#include <cstddef>
#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/utf_string_conversions.h"
#include "base/version_info/channel.h"
#include "base/version_info/version_info.h"
#include "brave/browser/brave_ads/ad_units/notification_ad/notification_ad_platform_bridge.h"
#include "brave/browser/brave_ads/application_state/notification_helper/notification_helper.h"
#include "brave/browser/ui/brave_ads/notification_ad.h"
#include "brave/components/brave_adaptive_captcha/brave_adaptive_captcha_service.h"
#include "brave/components/l10n/common/locale_util.h"
#include "brave/components/skus/browser/pref_names.h"
#include "build/build_config.h"
#include "chrome/browser/notifications/notification_display_service.h"
#include "chrome/browser/notifications/notification_display_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/channel_info.h"
#include "components/search_engines/template_url_prepopulate_data.h"
#include "ui/message_center/public/cpp/notification.h"
#include "ui/message_center/public/cpp/notification_types.h"
#include "ui/message_center/public/cpp/notifier_id.h"

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

constexpr char kSkuEnvironmentPrefix[] = "skus:";
constexpr char kSkuOrdersKey[] = "orders";
constexpr char kSkuOrderLocationKey[] = "location";
constexpr char kSkuOrderCreatedAtKey[] = "created_at";
constexpr char kSkuOrderExpiresAtKey[] = "expires_at";
constexpr char kSkuOrderLastPaidAtKey[] = "last_paid_at";
constexpr char kSkuOrderStatusKey[] = "status";
constexpr char kNotificationAdUrlPrefix[] = "https://www.brave.com/ads/?";

std::string StripSkuEnvironmentPrefix(const std::string& environment) {
  const size_t pos = environment.find(':');
  return environment.substr(pos + 1);
}

std::string NormalizeSkuStatus(const std::string& status) {
  return status == "cancelled" ? "canceled" : status;
}

base::Value::Dict ParseSkuOrder(const base::Value::Dict& dict) {
  base::Value::Dict order;

  if (const auto* const created_at = dict.FindString(kSkuOrderCreatedAtKey)) {
    order.Set(kSkuOrderCreatedAtKey, *created_at);
  }

  if (const auto* const expires_at = dict.FindString(kSkuOrderExpiresAtKey)) {
    order.Set(kSkuOrderExpiresAtKey, *expires_at);
  }

  if (const auto* const last_paid_at =
          dict.FindString(kSkuOrderLastPaidAtKey)) {
    order.Set(kSkuOrderLastPaidAtKey, *last_paid_at);
  }

  if (const auto* const status = dict.FindString(kSkuOrderStatusKey)) {
    const std::string normalized_status = NormalizeSkuStatus(*status);
    order.Set(kSkuOrderStatusKey, normalized_status);
  }

  return order;
}

base::Value::Dict ParseSkuOrders(const base::Value::Dict& dict) {
  base::Value::Dict orders;

  for (const auto [/*id*/ _, value] : dict) {
    const base::Value::Dict* const order = value.GetIfDict();
    if (!order) {
      continue;
    }

    const std::string* const location = order->FindString(kSkuOrderLocationKey);
    if (!location) {
      continue;
    }

    orders.Set(*location, ParseSkuOrder(*order));
  }

  return orders;
}

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
      search_engine_choice_service_(
          *profile_->GetPrefs(),
          local_state_,
          /*is_profile_eligible_for_dse_guest_propagation=*/false),
      adaptive_captcha_service_(adaptive_captcha_service),
      notification_ad_platform_bridge_(
          std::move(notification_ad_platform_bridge)) {}

AdsServiceDelegate::~AdsServiceDelegate() {}

std::string AdsServiceDelegate::GetDefaultSearchEngineName() {
  const auto template_url_data =
      TemplateURLPrepopulateData::GetPrepopulatedFallbackSearch(
          profile_->GetPrefs(), &search_engine_choice_service_);

  const std::u16string& default_search_engine_name =
      template_url_data ? template_url_data->short_name() : u"";
  return base::UTF16ToUTF8(default_search_engine_name);
}

base::Value::Dict AdsServiceDelegate::GetSkus() const {
  base::Value::Dict skus;

  if (!local_state_->FindPreference(skus::prefs::kSkusState)) {
    // No SKUs in local state.
    return skus;
  }

  const base::Value::Dict& skus_state =
      local_state_->GetDict(skus::prefs::kSkusState);
  for (const auto [environment, value] : skus_state) {
    if (!environment.starts_with(kSkuEnvironmentPrefix)) {
      continue;
    }

    // Parse the SKUs JSON because it is stored as a string in local state.
    const std::optional<base::Value::Dict> sku_state =
        base::JSONReader::ReadDict(value.GetString());
    if (!sku_state) {
      continue;
    }

    const base::Value::Dict* const orders = sku_state->FindDict(kSkuOrdersKey);
    if (!orders) {
      continue;
    }

    skus.Set(StripSkuEnvironmentPrefix(environment), ParseSkuOrders(*orders));
  }

  return skus;
}

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
  nav_params.window_action = NavigateParams::SHOW_WINDOW;
  nav_params.path_behavior = NavigateParams::RESPECT;
  Navigate(&nav_params);
#endif
}

void AdsServiceDelegate::InitNotificationHelper() {
  NotificationHelper::GetInstance()->InitForProfile(&*profile_);
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
        NotificationAd(id, title, body, nullptr));
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

base::Value::Dict AdsServiceDelegate::GetVirtualPrefs() {
  return base::Value::Dict()
      .Set("[virtual]:browser",
           base::Value::Dict()
               .Set("build_channel",
                    version_info::GetChannelString(chrome::GetChannel()))
               .Set("version", version_info::GetVersionNumber()))
      .Set("[virtual]:operating_system",
           base::Value::Dict()
               .Set("locale",
                    base::Value::Dict()
                        .Set("language",
                             brave_l10n::GetDefaultISOLanguageCodeString())
                        .Set("region",
                             brave_l10n::GetDefaultISOCountryCodeString()))
               .Set("name", version_info::GetOSType()))
      .Set(
          "[virtual]:search_engine",
          base::Value::Dict().Set("default_name", GetDefaultSearchEngineName()))
      .Set("[virtual]:skus", GetSkus());
}

NotificationDisplayService*
AdsServiceDelegate::GetNotificationDisplayService() {
  return NotificationDisplayServiceFactory::GetForProfile(&*profile_);
}

}  // namespace brave_ads
