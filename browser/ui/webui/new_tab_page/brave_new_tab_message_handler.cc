// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/new_tab_page/brave_new_tab_message_handler.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/json/values_util.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_restrictions.h"
#include "base/values.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/ntp_background/view_counter_service_factory.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/components/brave_ads/core/public/ads_util.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_perf_predictor/common/pref_names.h"
#include "brave/components/brave_search_conversion/pref_names.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/ntp_background_images/browser/url_constants.h"
#include "brave/components/ntp_background_images/browser/view_counter_service.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "brave/components/p3a/utils.h"
#include "brave/components/time_period_storage/weekly_storage.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/first_run/first_run.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/plural_string_handler.h"
#include "chrome/common/pref_names.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_ui_data_source.h"

using ntp_background_images::ViewCounterServiceFactory;
using ntp_background_images::prefs::kBrandedWallpaperNotificationDismissed;
using ntp_background_images::prefs::kNewTabPageShowBackgroundImage;
using ntp_background_images::prefs::
    kNewTabPageShowSponsoredImagesBackgroundImage;  // NOLINT

namespace {

bool IsPrivateNewTab(Profile* profile) {
  return profile->IsIncognitoProfile() || profile->IsGuestSession();
}

base::Value::Dict GetStatsDictionary(PrefService* prefs) {
  base::Value::Dict stats_data;
  stats_data.Set("adsBlockedStat",
                 base::Int64ToValue(prefs->GetUint64(kAdsBlocked) +
                                    prefs->GetUint64(kTrackersBlocked)));
  stats_data.Set("javascriptBlockedStat",
                 base::Int64ToValue(prefs->GetUint64(kJavascriptBlocked)));
  stats_data.Set("fingerprintingBlockedStat",
                 base::Int64ToValue(prefs->GetUint64(kFingerprintingBlocked)));
  stats_data.Set("bandwidthSavedStat",
                 base::Int64ToValue(prefs->GetUint64(
                     brave_perf_predictor::prefs::kBandwidthSavedBytes)));
  return stats_data;
}

base::Value::Dict GetPreferencesDictionary(PrefService* prefs) {
  base::Value::Dict pref_data;
  pref_data.Set("showBackgroundImage",
                prefs->GetBoolean(kNewTabPageShowBackgroundImage));
  pref_data.Set(
      "brandedWallpaperOptIn",
      prefs->GetBoolean(kNewTabPageShowSponsoredImagesBackgroundImage));
  pref_data.Set("showClock", prefs->GetBoolean(kNewTabPageShowClock));
  pref_data.Set("clockFormat", prefs->GetString(kNewTabPageClockFormat));
  pref_data.Set("showStats", prefs->GetBoolean(kNewTabPageShowStats));
  pref_data.Set("showToday",
                prefs->GetBoolean(brave_news::prefs::kNewTabPageShowToday));
  pref_data.Set("showRewards", prefs->GetBoolean(kNewTabPageShowRewards));
  pref_data.Set("isBrandedWallpaperNotificationDismissed",
                prefs->GetBoolean(kBrandedWallpaperNotificationDismissed));
  pref_data.Set("isBraveNewsOptedIn",
                prefs->GetBoolean(brave_news::prefs::kBraveNewsOptedIn));
  pref_data.Set("hideAllWidgets", prefs->GetBoolean(kNewTabPageHideAllWidgets));
  pref_data.Set("showBraveTalk", prefs->GetBoolean(kNewTabPageShowBraveTalk));
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  pref_data.Set("showBraveVPN", prefs->GetBoolean(kNewTabPageShowBraveVPN));
#endif
  pref_data.Set(
      "showSearchBox",
      prefs->GetBoolean(brave_search_conversion::prefs::kShowNTPSearchBox));
  pref_data.Set("lastUsedNtpSearchEngine",
                prefs->GetString(
                    brave_search_conversion::prefs::kLastUsedNTPSearchEngine));
  pref_data.Set("promptEnableSearchSuggestions",
                prefs->GetBoolean(
                    brave_search_conversion::prefs::kPromptEnableSuggestions));
  pref_data.Set("searchSuggestionsEnabled",
                prefs->GetBoolean(prefs::kSearchSuggestEnabled));
  return pref_data;
}

// TODO(petemill): Move p3a to own NTP component so it can
// be used by other platforms.

enum class NTPCustomizeUsage { kNeverOpened, kOpened, kOpenedAndEdited, kSize };

constexpr char kNTPCustomizeUsageStatus[] =
    "brave.new_tab_page.customize_p3a_usage";
constexpr char kCustomizeUsageHistogramName[] =
    "Brave.NTP.CustomizeUsageStatus.2";

constexpr char kNeedsBrowserUpgradeToServeAds[] =
    "needsBrowserUpgradeToServeAds";

}  // namespace

// static
void BraveNewTabMessageHandler::RegisterLocalStatePrefs(
    PrefRegistrySimple* local_state) {
  local_state->RegisterIntegerPref(kNTPCustomizeUsageStatus, -1);
}

void BraveNewTabMessageHandler::RecordInitialP3AValues(
    PrefService* local_state) {
  p3a::RecordValueIfGreater<NTPCustomizeUsage>(
      NTPCustomizeUsage::kNeverOpened, kCustomizeUsageHistogramName,
      kNTPCustomizeUsageStatus, local_state);
}

// static
BraveNewTabMessageHandler* BraveNewTabMessageHandler::Create(
    content::WebUIDataSource* source,
    Profile* profile,
    bool was_restored) {
  //
  // Initial Values
  // Should only contain data that is static
  //
  auto* ads_service = brave_ads::AdsServiceFactory::GetForProfile(profile);
  // For safety, default |is_ads_supported_locale_| to true. Better to have
  // false positive than falsen egative,
  // in which case we would not show "opt out" toggle.
  bool is_ads_supported_locale = true;
  if (!ads_service) {
    LOG(ERROR) << "Ads service is not initialized!";
  } else {
    is_ads_supported_locale = brave_ads::IsSupportedRegion();
  }

  source->AddBoolean("featureFlagBraveNTPSponsoredImagesWallpaper",
                     is_ads_supported_locale);

  // Private Tab info
  if (IsPrivateNewTab(profile)) {
    source->AddBoolean("isTor", profile->IsTor());
  }
  return new BraveNewTabMessageHandler(profile, was_restored);
}

BraveNewTabMessageHandler::BraveNewTabMessageHandler(Profile* profile,
                                                     bool was_restored)
    : profile_(profile), was_restored_(was_restored), weak_ptr_factory_(this) {
  ads_service_ = brave_ads::AdsServiceFactory::GetForProfile(profile_);
}

BraveNewTabMessageHandler::~BraveNewTabMessageHandler() = default;

void BraveNewTabMessageHandler::RegisterMessages() {
  // TODO(petemill): This MessageHandler can be split up to
  // individual MessageHandlers for each individual topic area,
  // should other WebUI pages wish to consume the APIs:
  // - Stats
  // - Preferences
  // - PrivatePage properties
  auto plural_string_handler = std::make_unique<PluralStringHandler>();
  plural_string_handler->AddLocalizedString("braveNewsSourceCount",
                                            IDS_BRAVE_NEWS_SOURCE_COUNT);
  plural_string_handler->AddLocalizedString("rewardsPublisherCountText",
                                            IDS_REWARDS_PUBLISHER_COUNT_TEXT);
  web_ui()->AddMessageHandler(std::move(plural_string_handler));

  web_ui()->RegisterMessageCallback(
      "getNewTabPagePreferences",
      base::BindRepeating(&BraveNewTabMessageHandler::HandleGetPreferences,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getNewTabPageStats",
      base::BindRepeating(&BraveNewTabMessageHandler::HandleGetStats,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getNewTabAdsData",
      base::BindRepeating(&BraveNewTabMessageHandler::HandleGetNewTabAdsData,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "saveNewTabPagePref",
      base::BindRepeating(&BraveNewTabMessageHandler::HandleSaveNewTabPagePref,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "registerNewTabPageView",
      base::BindRepeating(
          &BraveNewTabMessageHandler::HandleRegisterNewTabPageView,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "brandedWallpaperLogoClicked",
      base::BindRepeating(
          &BraveNewTabMessageHandler::HandleBrandedWallpaperLogoClicked,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getWallpaperData",
      base::BindRepeating(&BraveNewTabMessageHandler::HandleGetWallpaperData,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "customizeClicked",
      base::BindRepeating(&BraveNewTabMessageHandler::HandleCustomizeClicked,
                          base::Unretained(this)));
}

void BraveNewTabMessageHandler::OnJavascriptAllowed() {
  // Observe relevant preferences
  PrefService* prefs = profile_->GetPrefs();
  pref_change_registrar_.Init(prefs);
  // Stats
  pref_change_registrar_.Add(
      kAdsBlocked,
      base::BindRepeating(&BraveNewTabMessageHandler::OnStatsChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      kTrackersBlocked,
      base::BindRepeating(&BraveNewTabMessageHandler::OnStatsChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      kJavascriptBlocked,
      base::BindRepeating(&BraveNewTabMessageHandler::OnStatsChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      kHttpsUpgrades,
      base::BindRepeating(&BraveNewTabMessageHandler::OnStatsChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      kFingerprintingBlocked,
      base::BindRepeating(&BraveNewTabMessageHandler::OnStatsChanged,
                          base::Unretained(this)));
  // News
  pref_change_registrar_.Add(
      brave_news::prefs::kBraveNewsOptedIn,
      base::BindRepeating(&BraveNewTabMessageHandler::OnPreferencesChanged,
                          base::Unretained(this)));
  // New Tab Page preferences
  pref_change_registrar_.Add(
      kNewTabPageShowBackgroundImage,
      base::BindRepeating(&BraveNewTabMessageHandler::OnPreferencesChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      kNewTabPageShowSponsoredImagesBackgroundImage,
      base::BindRepeating(&BraveNewTabMessageHandler::OnPreferencesChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      brave_search_conversion::prefs::kShowNTPSearchBox,
      base::BindRepeating(&BraveNewTabMessageHandler::OnPreferencesChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      brave_search_conversion::prefs::kLastUsedNTPSearchEngine,
      base::BindRepeating(&BraveNewTabMessageHandler::OnPreferencesChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      brave_search_conversion::prefs::kPromptEnableSuggestions,
      base::BindRepeating(&BraveNewTabMessageHandler::OnPreferencesChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      prefs::kSearchSuggestEnabled,
      base::BindRepeating(&BraveNewTabMessageHandler::OnPreferencesChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      kNewTabPageShowClock,
      base::BindRepeating(&BraveNewTabMessageHandler::OnPreferencesChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      kNewTabPageClockFormat,
      base::BindRepeating(&BraveNewTabMessageHandler::OnPreferencesChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      kNewTabPageShowStats,
      base::BindRepeating(&BraveNewTabMessageHandler::OnPreferencesChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      brave_news::prefs::kNewTabPageShowToday,
      base::BindRepeating(&BraveNewTabMessageHandler::OnPreferencesChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      kNewTabPageShowRewards,
      base::BindRepeating(&BraveNewTabMessageHandler::OnPreferencesChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      kBrandedWallpaperNotificationDismissed,
      base::BindRepeating(&BraveNewTabMessageHandler::OnPreferencesChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      kNewTabPageShowBraveTalk,
      base::BindRepeating(&BraveNewTabMessageHandler::OnPreferencesChanged,
                          base::Unretained(this)));
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  pref_change_registrar_.Add(
      kNewTabPageShowBraveVPN,
      base::BindRepeating(&BraveNewTabMessageHandler::OnPreferencesChanged,
                          base::Unretained(this)));
#endif
  pref_change_registrar_.Add(
      kNewTabPageHideAllWidgets,
      base::BindRepeating(&BraveNewTabMessageHandler::OnPreferencesChanged,
                          base::Unretained(this)));

  bat_ads_observer_receiver_.reset();
  if (ads_service_) {
    ads_service_->AddBatAdsObserver(
        bat_ads_observer_receiver_.BindNewPipeAndPassRemote());
  }
}

void BraveNewTabMessageHandler::OnJavascriptDisallowed() {
  pref_change_registrar_.RemoveAll();
  bat_ads_observer_receiver_.reset();
  weak_ptr_factory_.InvalidateWeakPtrs();
}

void BraveNewTabMessageHandler::HandleGetPreferences(
    const base::Value::List& args) {
  AllowJavascript();
  PrefService* prefs = profile_->GetPrefs();
  auto data = GetPreferencesDictionary(prefs);
  ResolveJavascriptCallback(args[0], data);
}

void BraveNewTabMessageHandler::HandleGetStats(const base::Value::List& args) {
  AllowJavascript();
  PrefService* prefs = profile_->GetPrefs();
  auto data = GetStatsDictionary(prefs);
  ResolveJavascriptCallback(args[0], data);
}

void BraveNewTabMessageHandler::HandleGetNewTabAdsData(
    const base::Value::List& args) {
  AllowJavascript();

  ResolveJavascriptCallback(args[0], GetAdsDataDictionary());
}

void BraveNewTabMessageHandler::HandleSaveNewTabPagePref(
    const base::Value::List& args) {
  if (args.size() != 2) {
    LOG(ERROR) << "Invalid input";
    return;
  }
  PrefService* prefs = profile_->GetPrefs();
  // Collect args
  std::string settings_key_input = args[0].GetString();
  auto settings_value = args[1].Clone();
  std::string settings_key;

  // Prevent News onboarding below NTP and sponsored NTP notification
  // state from triggering the "shown & changed" answer for the
  // customize dialog metric.
  if (settings_key_input != "showToday" &&
      settings_key_input != "isBraveNewsOptedIn" &&
      settings_key_input != "isBrandedWallpaperNotificationDismissed") {
    p3a::RecordValueIfGreater<NTPCustomizeUsage>(
        NTPCustomizeUsage::kOpenedAndEdited, kCustomizeUsageHistogramName,
        kNTPCustomizeUsageStatus, g_browser_process->local_state());
  }

  // Handle string settings
  if (settings_value.is_string()) {
    const auto settings_value_string = settings_value.GetString();
    if (settings_key_input == "clockFormat") {
      settings_key = kNewTabPageClockFormat;
    } else if (settings_key_input == "lastUsedNtpSearchEngine") {
      settings_key = brave_search_conversion::prefs::kLastUsedNTPSearchEngine;
    } else {
      LOG(ERROR) << "Invalid setting key";
      return;
    }
    prefs->SetString(settings_key, settings_value_string);
    return;
  }

  // Handle bool settings
  if (!settings_value.is_bool()) {
    LOG(ERROR) << "Invalid value type";
    return;
  }
  const auto settings_value_bool = settings_value.GetBool();
  if (settings_key_input == "showBackgroundImage") {
    settings_key = kNewTabPageShowBackgroundImage;
  } else if (settings_key_input == "brandedWallpaperOptIn") {
    // TODO(simonhong): I think above |brandedWallpaperOptIn| should be changed
    // to |sponsoredImagesWallpaperOptIn|.
    settings_key = kNewTabPageShowSponsoredImagesBackgroundImage;
  } else if (settings_key_input == "showClock") {
    settings_key = kNewTabPageShowClock;
  } else if (settings_key_input == "showStats") {
    settings_key = kNewTabPageShowStats;
  } else if (settings_key_input == "showToday") {
    settings_key = brave_news::prefs::kNewTabPageShowToday;
  } else if (settings_key_input == "isBraveNewsOptedIn") {
    settings_key = brave_news::prefs::kBraveNewsOptedIn;
  } else if (settings_key_input == "showRewards") {
    settings_key = kNewTabPageShowRewards;
  } else if (settings_key_input == "isBrandedWallpaperNotificationDismissed") {
    settings_key = kBrandedWallpaperNotificationDismissed;
  } else if (settings_key_input == "hideAllWidgets") {
    settings_key = kNewTabPageHideAllWidgets;
  } else if (settings_key_input == "showBraveTalk") {
    settings_key = kNewTabPageShowBraveTalk;
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  } else if (settings_key_input == "showBraveVPN") {
    settings_key = kNewTabPageShowBraveVPN;
#endif
  } else if (settings_key_input == "showSearchBox") {
    settings_key = brave_search_conversion::prefs::kShowNTPSearchBox;
  } else if (settings_key_input == "promptEnableSearchSuggestions") {
    settings_key = brave_search_conversion::prefs::kPromptEnableSuggestions;
  } else if (settings_key_input == "searchSuggestionsEnabled") {
    settings_key = prefs::kSearchSuggestEnabled;
  } else {
    LOG(ERROR) << "Invalid setting key";
    return;
  }
  prefs->SetBoolean(settings_key, settings_value_bool);
}

void BraveNewTabMessageHandler::HandleRegisterNewTabPageView(
    const base::Value::List& args) {
  AllowJavascript();

  // Decrement original value only if there's actual branded content and we are
  // not restoring browser tabs.
  if (was_restored_) {
    was_restored_ = false;
    return;
  }

  if (auto* service = ViewCounterServiceFactory::GetForProfile(profile_)) {
    service->RegisterPageView();
  }
}

void BraveNewTabMessageHandler::HandleBrandedWallpaperLogoClicked(
    const base::Value::List& args) {
  AllowJavascript();
  if (args.size() != 1) {
    LOG(ERROR) << "Invalid input";
    return;
  }

  if (auto* service = ViewCounterServiceFactory::GetForProfile(profile_)) {
    const auto& arg = args[0].GetDict();
    auto* creative_instance_id =
        arg.FindString(ntp_background_images::kCreativeInstanceIDKey);
    auto* destination_url = arg.FindStringByDottedPath(
        ntp_background_images::kLogoDestinationURLPath);
    auto* wallpaper_id =
        arg.FindStringByDottedPath(ntp_background_images::kWallpaperIDKey);

    DCHECK(creative_instance_id);
    DCHECK(destination_url);
    DCHECK(wallpaper_id);

    service->BrandedWallpaperLogoClicked(
        creative_instance_id ? *creative_instance_id : "",
        destination_url ? *destination_url : "",
        wallpaper_id ? *wallpaper_id : "");
  }
}

void BraveNewTabMessageHandler::HandleGetWallpaperData(
    const base::Value::List& args) {
  AllowJavascript();

  auto* service = ViewCounterServiceFactory::GetForProfile(profile_);
  base::Value::Dict wallpaper;

  if (!service) {
    ResolveJavascriptCallback(args[0], wallpaper);
    return;
  }

  std::optional<base::Value::Dict> data =
      was_restored_ ? service->GetNextWallpaperForDisplay()
                    : service->GetCurrentWallpaperForDisplay();

  if (!data) {
    ResolveJavascriptCallback(args[0], wallpaper);
    return;
  }

  const auto is_background =
      data->FindBool(ntp_background_images::kIsBackgroundKey);
  DCHECK(is_background);

  constexpr char kBackgroundWallpaperKey[] = "backgroundWallpaper";
  if (is_background.value()) {
    wallpaper.Set(kBackgroundWallpaperKey, std::move(*data));
    ResolveJavascriptCallback(args[0], wallpaper);
    return;
  }

  // Even though we show sponsored image, we should pass "Background wallpaper"
  // data so that NTP customization menu can know which wallpaper is selected by
  // users.
  auto background_wallpaper = service->GetCurrentWallpaper();
  wallpaper.Set(kBackgroundWallpaperKey,
                background_wallpaper
                    ? base::Value(std::move(*background_wallpaper))
                    : base::Value());

  const std::string* creative_instance_id =
      data->FindString(ntp_background_images::kCreativeInstanceIDKey);
  const std::string* wallpaper_id =
      data->FindString(ntp_background_images::kWallpaperIDKey);
  const std::string* campaign_id =
      data->FindString(ntp_background_images::kCampaignIdKey);
  service->BrandedWallpaperWillBeDisplayed(
      wallpaper_id ? *wallpaper_id : "",
      creative_instance_id ? *creative_instance_id : "",
      campaign_id ? *campaign_id : "");

  constexpr char kBrandedWallpaperKey[] = "brandedWallpaper";
  wallpaper.Set(kBrandedWallpaperKey, std::move(*data));
  ResolveJavascriptCallback(args[0], wallpaper);
}

void BraveNewTabMessageHandler::HandleCustomizeClicked(
    const base::Value::List& args) {
  AllowJavascript();
  p3a::RecordValueIfGreater<NTPCustomizeUsage>(
      NTPCustomizeUsage::kOpened, kCustomizeUsageHistogramName,
      kNTPCustomizeUsageStatus, g_browser_process->local_state());
}

void BraveNewTabMessageHandler::OnStatsChanged() {
  PrefService* prefs = profile_->GetPrefs();
  auto data = GetStatsDictionary(prefs);
  FireWebUIListener("stats-updated", data);
}

void BraveNewTabMessageHandler::OnPreferencesChanged() {
  PrefService* prefs = profile_->GetPrefs();
  auto data = GetPreferencesDictionary(prefs);
  FireWebUIListener("preferences-changed", data);
}

base::Value::Dict BraveNewTabMessageHandler::GetAdsDataDictionary() const {
  if (!ads_service_) {
    return {};
  }

  return base::Value::Dict().Set(
      kNeedsBrowserUpgradeToServeAds,
      ads_service_->IsBrowserUpgradeRequiredToServeAds());
}

void BraveNewTabMessageHandler::OnBrowserUpgradeRequiredToServeAds() {
  FireWebUIListener("new-tab-ads-data-updated", GetAdsDataDictionary());
}
