// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/new_tab_page/brave_new_tab_message_handler.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/json/json_writer.h"
#include "base/json/values_util.h"
#include "base/memory/weak_ptr.h"
#include "base/metrics/histogram_macros.h"
#include "base/threading/thread_restrictions.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/ntp_background/view_counter_service_factory.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/search_engines/pref_names.h"
#include "brave/browser/search_engines/search_engine_provider_util.h"
#include "brave/browser/ui/webui/new_tab_page/brave_new_tab_ui.h"
#include "brave/components/brave_perf_predictor/common/pref_names.h"
#include "brave/components/brave_today/common/pref_names.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/crypto_dot_com/browser/buildflags/buildflags.h"
#include "brave/components/ftx/browser/buildflags/buildflags.h"
#include "brave/components/ntp_background_images/browser/url_constants.h"
#include "brave/components/ntp_background_images/browser/view_counter_service.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "brave/components/p3a/brave_p3a_utils.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "brave/components/time_period_storage/weekly_storage.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/first_run/first_run.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_features.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_ui_data_source.h"

using ntp_background_images::ViewCounterServiceFactory;
using ntp_background_images::prefs::kBrandedWallpaperNotificationDismissed;
using ntp_background_images::prefs::kNewTabPageShowBackgroundImage;
using ntp_background_images::prefs::
    kNewTabPageShowSponsoredImagesBackgroundImage;  // NOLINT

#if BUILDFLAG(CRYPTO_DOT_COM_ENABLED)
#include "brave/components/crypto_dot_com/common/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_FTX)
#include "brave/components/ftx/common/pref_names.h"
#endif

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
  pref_data.Set("showBraveNewsButton",
                prefs->GetBoolean(brave_news::prefs::kShouldShowToolbarButton));
  pref_data.Set("showRewards", prefs->GetBoolean(kNewTabPageShowRewards));
  pref_data.Set("isBrandedWallpaperNotificationDismissed",
                prefs->GetBoolean(kBrandedWallpaperNotificationDismissed));
  pref_data.Set("isBraveTodayOptedIn",
                prefs->GetBoolean(brave_news::prefs::kBraveTodayOptedIn));
  pref_data.Set("hideAllWidgets", prefs->GetBoolean(kNewTabPageHideAllWidgets));
  pref_data.Set("showBinance", prefs->GetBoolean(kNewTabPageShowBinance));
  pref_data.Set("showBraveTalk", prefs->GetBoolean(kNewTabPageShowBraveTalk));
  pref_data.Set("showGemini", prefs->GetBoolean(kNewTabPageShowGemini));
#if BUILDFLAG(CRYPTO_DOT_COM_ENABLED)
  pref_data.Set("showCryptoDotCom",
                prefs->GetBoolean(kCryptoDotComNewTabPageShowCryptoDotCom));
#endif
#if BUILDFLAG(ENABLE_FTX)
  pref_data.Set("showFTX", prefs->GetBoolean(kFTXNewTabPageShowFTX));
#endif
  return pref_data;
}

base::Value::Dict GetPrivatePropertiesDictionary(PrefService* prefs) {
  base::Value::Dict private_data;
  private_data.Set(
      "useAlternativePrivateSearchEngine",
      prefs->GetBoolean(kUseAlternativePrivateSearchEngineProvider));
  private_data.Set(
      "showAlternativePrivateSearchEngineToggle",
      prefs->GetBoolean(kShowAlternativePrivateSearchEngineProviderToggle));
  return private_data;
}

// TODO(petemill): Move p3a to own NTP component so it can
// be used by other platforms.

enum class NTPCustomizeUsage { kNeverOpened, kOpened, kOpenedAndEdited, kSize };

const char kNTPCustomizeUsageStatus[] =
    "brave.new_tab_page.customize_p3a_usage";

const char kNeedsBrowserUpgradeToServeAds[] = "needsBrowserUpgradeToServeAds";

}  // namespace

// static
void BraveNewTabMessageHandler::RegisterLocalStatePrefs(
    PrefRegistrySimple* local_state) {
  local_state->RegisterIntegerPref(kNTPCustomizeUsageStatus, -1);
}

void BraveNewTabMessageHandler::RecordInitialP3AValues(
    PrefService* local_state) {
  brave::RecordValueIfGreater<NTPCustomizeUsage>(
      NTPCustomizeUsage::kNeverOpened, "Brave.NTP.CustomizeUsageStatus",
      kNTPCustomizeUsageStatus, local_state);
}

bool BraveNewTabMessageHandler::CanPromptBraveTalk() {
  return BraveNewTabMessageHandler::CanPromptBraveTalk(base::Time::Now());
}

bool BraveNewTabMessageHandler::CanPromptBraveTalk(base::Time now) {
  // Only show Brave Talk prompt 4 days after first run.
  // CreateSentinelIfNeeded() is called in chrome_browser_main.cc, making this a
  // non-blocking read of the cached sentinel value when running from production
  // code. However tests will never create the sentinel file due to being run
  // with the switches:kNoFirstRun flag, so we need to allow blocking for that.
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::Time time_first_run = first_run::GetFirstRunSentinelCreationTime();
  base::Time talk_prompt_trigger_time = now - base::Days(3);
  return (time_first_run <= talk_prompt_trigger_time);
}

// static
BraveNewTabMessageHandler* BraveNewTabMessageHandler::Create(
    content::WebUIDataSource* source,
    Profile* profile) {
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
    is_ads_supported_locale = ads_service->IsSupportedLocale();
  }

  source->AddBoolean("featureFlagBraveNTPSponsoredImagesWallpaper",
                     is_ads_supported_locale);
  source->AddBoolean("braveTalkPromptAllowed",
                     BraveNewTabMessageHandler::CanPromptBraveTalk());

  // Private Tab info
  if (IsPrivateNewTab(profile)) {
    source->AddBoolean("isTor", profile->IsTor());
    source->AddBoolean("isQwant", brave::IsRegionForQwant(profile));
  }
  return new BraveNewTabMessageHandler(profile);
}

BraveNewTabMessageHandler::BraveNewTabMessageHandler(Profile* profile)
    : profile_(profile), weak_ptr_factory_(this) {
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
  web_ui()->RegisterMessageCallback(
      "getNewTabPagePreferences",
      base::BindRepeating(&BraveNewTabMessageHandler::HandleGetPreferences,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getNewTabPageStats",
      base::BindRepeating(&BraveNewTabMessageHandler::HandleGetStats,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getNewTabPagePrivateProperties",
      base::BindRepeating(
          &BraveNewTabMessageHandler::HandleGetPrivateProperties,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getNewTabAdsData",
      base::BindRepeating(&BraveNewTabMessageHandler::HandleGetNewTabAdsData,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "toggleAlternativePrivateSearchEngine",
      base::BindRepeating(&BraveNewTabMessageHandler::
                              HandleToggleAlternativeSearchEngineProvider,
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

  if (IsPrivateNewTab(profile_)) {
    // Private New Tab Page preferences
    pref_change_registrar_.Add(
        kUseAlternativePrivateSearchEngineProvider,
        base::BindRepeating(
            &BraveNewTabMessageHandler::OnPrivatePropertiesChanged,
            base::Unretained(this)));
  }
  // News
  pref_change_registrar_.Add(
      brave_news::prefs::kBraveTodayOptedIn,
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
      brave_news::prefs::kShouldShowToolbarButton,
      base::BindRepeating(&BraveNewTabMessageHandler::OnPreferencesChanged,
                          base::Unretained(this))),
      pref_change_registrar_.Add(
          kNewTabPageShowRewards,
          base::BindRepeating(&BraveNewTabMessageHandler::OnPreferencesChanged,
                              base::Unretained(this)));
  pref_change_registrar_.Add(
      kBrandedWallpaperNotificationDismissed,
      base::BindRepeating(&BraveNewTabMessageHandler::OnPreferencesChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      kNewTabPageShowBinance,
      base::BindRepeating(&BraveNewTabMessageHandler::OnPreferencesChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      kNewTabPageShowBraveTalk,
      base::BindRepeating(&BraveNewTabMessageHandler::OnPreferencesChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      kNewTabPageShowGemini,
      base::BindRepeating(&BraveNewTabMessageHandler::OnPreferencesChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      kNewTabPageHideAllWidgets,
      base::BindRepeating(&BraveNewTabMessageHandler::OnPreferencesChanged,
                          base::Unretained(this)));
#if BUILDFLAG(CRYPTO_DOT_COM_ENABLED)
  pref_change_registrar_.Add(
      kCryptoDotComNewTabPageShowCryptoDotCom,
      base::BindRepeating(&BraveNewTabMessageHandler::OnPreferencesChanged,
                          base::Unretained(this)));
#endif
#if BUILDFLAG(ENABLE_FTX)
  pref_change_registrar_.Add(
      kFTXNewTabPageShowFTX,
      base::BindRepeating(&BraveNewTabMessageHandler::OnPreferencesChanged,
                          base::Unretained(this)));
#endif

  if (ads_service_) {
    ads_service_observation_.Reset();
    ads_service_observation_.Observe(ads_service_);
  }
}

void BraveNewTabMessageHandler::OnJavascriptDisallowed() {
  pref_change_registrar_.RemoveAll();
  ads_service_observation_.Reset();
  weak_ptr_factory_.InvalidateWeakPtrs();
}

void BraveNewTabMessageHandler::HandleGetPreferences(
    const base::Value::List& args) {
  AllowJavascript();
  PrefService* prefs = profile_->GetPrefs();
  auto data = GetPreferencesDictionary(prefs);
  ResolveJavascriptCallback(args[0], base::Value(std::move(data)));
}

void BraveNewTabMessageHandler::HandleGetStats(const base::Value::List& args) {
  AllowJavascript();
  PrefService* prefs = profile_->GetPrefs();
  auto data = GetStatsDictionary(prefs);
  ResolveJavascriptCallback(args[0], base::Value(std::move(data)));
}

void BraveNewTabMessageHandler::HandleGetPrivateProperties(
    const base::Value::List& args) {
  AllowJavascript();
  PrefService* prefs = profile_->GetPrefs();
  auto data = GetPrivatePropertiesDictionary(prefs);
  ResolveJavascriptCallback(args[0], base::Value(std::move(data)));
}

void BraveNewTabMessageHandler::HandleGetNewTabAdsData(
    const base::Value::List& args) {
  if (!ads_service_) {
    return;
  }

  AllowJavascript();

  base::Value data = GetAdsDataDictionary();
  ResolveJavascriptCallback(args[0], std::move(data));
}

void BraveNewTabMessageHandler::HandleToggleAlternativeSearchEngineProvider(
    const base::Value::List& args) {
  // Alternative search related code will not be used.
  // Cleanup "toggleAlternativePrivateSearchEngine" message handler when it's
  // deleted from NTP Webui.
  // https://github.com/brave/brave-browser/issues/23493
  NOTREACHED();
}

void BraveNewTabMessageHandler::HandleSaveNewTabPagePref(
    const base::Value::List& args) {
  if (args.size() != 2) {
    LOG(ERROR) << "Invalid input";
    return;
  }
  brave::RecordValueIfGreater<NTPCustomizeUsage>(
      NTPCustomizeUsage::kOpenedAndEdited, "Brave.NTP.CustomizeUsageStatus",
      kNTPCustomizeUsageStatus, g_browser_process->local_state());
  PrefService* prefs = profile_->GetPrefs();
  // Collect args
  std::string settingsKeyInput = args[0].GetString();
  auto settingsValue = args[1].Clone();
  std::string settingsKey;

  // Handle string settings
  if (settingsValue.is_string()) {
    const auto settingsValueString = settingsValue.GetString();
    if (settingsKeyInput == "clockFormat") {
      settingsKey = kNewTabPageClockFormat;
    } else {
      LOG(ERROR) << "Invalid setting key";
      return;
    }
    prefs->SetString(settingsKey, settingsValueString);
    return;
  }

  // Handle bool settings
  if (!settingsValue.is_bool()) {
    LOG(ERROR) << "Invalid value type";
    return;
  }
  const auto settingsValueBool = settingsValue.GetBool();
  if (settingsKeyInput == "showBackgroundImage") {
    settingsKey = kNewTabPageShowBackgroundImage;
  } else if (settingsKeyInput == "brandedWallpaperOptIn") {
    // TODO(simonhong): I think above |brandedWallpaperOptIn| should be changed
    // to |sponsoredImagesWallpaperOptIn|.
    settingsKey = kNewTabPageShowSponsoredImagesBackgroundImage;
  } else if (settingsKeyInput == "showClock") {
    settingsKey = kNewTabPageShowClock;
  } else if (settingsKeyInput == "showStats") {
    settingsKey = kNewTabPageShowStats;
  } else if (settingsKeyInput == "showToday") {
    settingsKey = brave_news::prefs::kNewTabPageShowToday;
  } else if (settingsKeyInput == "showBraveNewsButton") {
    settingsKey = brave_news::prefs::kShouldShowToolbarButton;
  } else if (settingsKeyInput == "isBraveTodayOptedIn") {
    settingsKey = brave_news::prefs::kBraveTodayOptedIn;
  } else if (settingsKeyInput == "showRewards") {
    settingsKey = kNewTabPageShowRewards;
  } else if (settingsKeyInput == "isBrandedWallpaperNotificationDismissed") {
    settingsKey = kBrandedWallpaperNotificationDismissed;
  } else if (settingsKeyInput == "hideAllWidgets") {
    settingsKey = kNewTabPageHideAllWidgets;
  } else if (settingsKeyInput == "showBinance") {
    settingsKey = kNewTabPageShowBinance;
  } else if (settingsKeyInput == "showBraveTalk") {
    settingsKey = kNewTabPageShowBraveTalk;
  } else if (settingsKeyInput == "showGemini") {
    settingsKey = kNewTabPageShowGemini;
#if BUILDFLAG(CRYPTO_DOT_COM_ENABLED)
  } else if (settingsKeyInput == "showCryptoDotCom") {
    settingsKey = kCryptoDotComNewTabPageShowCryptoDotCom;
#endif
#if BUILDFLAG(ENABLE_FTX)
  } else if (settingsKeyInput == "showFTX") {
    settingsKey = kFTXNewTabPageShowFTX;
#endif
  } else {
    LOG(ERROR) << "Invalid setting key";
    return;
  }
  prefs->SetBoolean(settingsKey, settingsValueBool);

  // P3A can only be recorded after profile is updated
  if (settingsKeyInput == "showBackgroundImage" ||
      settingsKeyInput == "brandedWallpaperOptIn") {
    brave::RecordSponsoredImagesEnabledP3A(profile_);
  }
}

void BraveNewTabMessageHandler::HandleRegisterNewTabPageView(
    const base::Value::List& args) {
  AllowJavascript();

  // Decrement original value only if there's actual branded content
  if (auto* service = ViewCounterServiceFactory::GetForProfile(profile_))
    service->RegisterPageView();
}

void BraveNewTabMessageHandler::HandleBrandedWallpaperLogoClicked(
    const base::Value::List& args) {
  AllowJavascript();
  if (args.size() != 1) {
    LOG(ERROR) << "Invalid input";
    return;
  }

  if (auto* service = ViewCounterServiceFactory::GetForProfile(profile_)) {
    auto* creative_instance_id =
        args[0].FindStringKey(ntp_background_images::kCreativeInstanceIDKey);
    auto* destination_url =
        args[0].FindStringPath(ntp_background_images::kLogoDestinationURLPath);
    auto* wallpaper_id =
        args[0].FindStringPath(ntp_background_images::kWallpaperIDKey);

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
    ResolveJavascriptCallback(args[0], base::Value(std::move(wallpaper)));
    return;
  }

  absl::optional<base::Value::Dict> data =
      service->GetCurrentWallpaperForDisplay();

  if (!data) {
    ResolveJavascriptCallback(args[0], base::Value(std::move(wallpaper)));
    return;
  }

  const auto is_background =
      data->FindBool(ntp_background_images::kIsBackgroundKey);
  DCHECK(is_background);

  constexpr char kBackgroundWallpaperKey[] = "backgroundWallpaper";
  if (is_background.value()) {
    wallpaper.Set(kBackgroundWallpaperKey, std::move(*data));
    ResolveJavascriptCallback(args[0], base::Value(std::move(wallpaper)));
    return;
  }

  // Even though we show sponsored image, we should pass "Background wallpaper"
  // data so that NTP can know which wallpaper is selected by users.
  auto backgroundWallpaper = service->GetCurrentWallpaper();
  wallpaper.Set(kBackgroundWallpaperKey,
                backgroundWallpaper
                    ? base::Value(std::move(*backgroundWallpaper))
                    : base::Value());

  const std::string* creative_instance_id =
      data->FindString(ntp_background_images::kCreativeInstanceIDKey);
  const std::string* wallpaper_id =
      data->FindString(ntp_background_images::kWallpaperIDKey);
  service->BrandedWallpaperWillBeDisplayed(wallpaper_id, creative_instance_id);

  constexpr char kBrandedWallpaperKey[] = "brandedWallpaper";
  wallpaper.Set(kBrandedWallpaperKey, std::move(*data));
  ResolveJavascriptCallback(args[0], base::Value(std::move(wallpaper)));
}

void BraveNewTabMessageHandler::HandleCustomizeClicked(
    const base::Value::List& args) {
  AllowJavascript();
  brave::RecordValueIfGreater<NTPCustomizeUsage>(
      NTPCustomizeUsage::kOpened, "Brave.NTP.CustomizeUsageStatus",
      kNTPCustomizeUsageStatus, g_browser_process->local_state());
}

void BraveNewTabMessageHandler::OnPrivatePropertiesChanged() {
  PrefService* prefs = profile_->GetPrefs();
  auto data = GetPrivatePropertiesDictionary(prefs);
  FireWebUIListener("private-tab-data-updated", base::Value(std::move(data)));
}

void BraveNewTabMessageHandler::OnStatsChanged() {
  PrefService* prefs = profile_->GetPrefs();
  auto data = GetStatsDictionary(prefs);
  FireWebUIListener("stats-updated", base::Value(std::move(data)));
}

void BraveNewTabMessageHandler::OnPreferencesChanged() {
  PrefService* prefs = profile_->GetPrefs();
  auto data = GetPreferencesDictionary(prefs);
  FireWebUIListener("preferences-changed", base::Value(std::move(data)));
}

base::Value BraveNewTabMessageHandler::GetAdsDataDictionary() const {
  base::Value::Dict ads_data;

  bool needs_browser_update_to_see_ads = false;
  if (ads_service_) {
    needs_browser_update_to_see_ads =
        ads_service_->NeedsBrowserUpgradeToServeAds();
  }
  ads_data.Set(kNeedsBrowserUpgradeToServeAds, needs_browser_update_to_see_ads);

  return base::Value(std::move(ads_data));
}

void BraveNewTabMessageHandler::OnNeedsBrowserUpgradeToServeAds() {
  base::Value data = GetAdsDataDictionary();
  FireWebUIListener("new-tab-ads-data-updated", std::move(data));
}
