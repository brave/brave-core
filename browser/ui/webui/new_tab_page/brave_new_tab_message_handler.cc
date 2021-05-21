// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/new_tab_page/brave_new_tab_message_handler.h"

#include <memory>
#include <utility>

#include "base/guid.h"
#include "base/json/json_writer.h"
#include "base/metrics/histogram_macros.h"
#include "base/values.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/ntp_background_images/view_counter_service_factory.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/search_engines/search_engine_provider_util.h"
#include "brave/browser/ui/webui/new_tab_page/brave_new_tab_ui.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_perf_predictor/browser/buildflags.h"
#include "brave/components/crypto_dot_com/browser/buildflags/buildflags.h"
#include "brave/components/ftx/browser/buildflags/buildflags.h"
#include "brave/components/ntp_background_images/browser/features.h"
#include "brave/components/ntp_background_images/browser/url_constants.h"
#include "brave/components/ntp_background_images/browser/view_counter_service.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "brave/components/p3a/brave_p3a_utils.h"
#include "brave/components/weekly_storage/weekly_storage.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_features.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_ui_data_source.h"

using ntp_background_images::features::kBraveNTPBrandedWallpaper;
using ntp_background_images::prefs::kNewTabPageShowBackgroundImage;
using ntp_background_images::prefs::kNewTabPageShowSponsoredImagesBackgroundImage;  // NOLINT
using ntp_background_images::prefs::kBrandedWallpaperNotificationDismissed;
using ntp_background_images::ViewCounterServiceFactory;

#if BUILDFLAG(ENABLE_BRAVE_PERF_PREDICTOR)
#include "brave/components/brave_perf_predictor/common/pref_names.h"
#endif

#if BUILDFLAG(CRYPTO_DOT_COM_ENABLED)
#include "brave/components/crypto_dot_com/common/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_FTX)
#include "brave/components/ftx/common/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_TOR)
#include "brave/components/tor/tor_launcher_factory.h"
#endif

namespace {

bool IsPrivateNewTab(Profile* profile) {
  return profile->IsIncognitoProfile() || profile->IsGuestSession();
}

base::DictionaryValue GetStatsDictionary(PrefService* prefs) {
  base::DictionaryValue stats_data;
  stats_data.SetInteger(
    "adsBlockedStat",
    prefs->GetUint64(kAdsBlocked) + prefs->GetUint64(kTrackersBlocked));
  stats_data.SetInteger(
    "javascriptBlockedStat",
    prefs->GetUint64(kJavascriptBlocked));
  stats_data.SetInteger(
    "fingerprintingBlockedStat",
    prefs->GetUint64(kFingerprintingBlocked));
#if BUILDFLAG(ENABLE_BRAVE_PERF_PREDICTOR)
  stats_data.SetDouble(
      "bandwidthSavedStat",
      prefs->GetUint64(brave_perf_predictor::prefs::kBandwidthSavedBytes));
#endif
  return stats_data;
}

base::DictionaryValue GetPreferencesDictionary(PrefService* prefs) {
  base::DictionaryValue pref_data;
  pref_data.SetBoolean(
      "showBackgroundImage",
      prefs->GetBoolean(kNewTabPageShowBackgroundImage));
  pref_data.SetBoolean(
      "brandedWallpaperOptIn",
      prefs->GetBoolean(kNewTabPageShowSponsoredImagesBackgroundImage));
  pref_data.SetBoolean(
      "showClock",
      prefs->GetBoolean(kNewTabPageShowClock));
  pref_data.SetString(
      "clockFormat",
      prefs->GetString(kNewTabPageClockFormat));
  pref_data.SetBoolean(
      "showStats",
      prefs->GetBoolean(kNewTabPageShowStats));
  pref_data.SetBoolean(
      "showToday",
      prefs->GetBoolean(kNewTabPageShowToday));
  pref_data.SetBoolean(
      "showRewards",
      prefs->GetBoolean(kNewTabPageShowRewards));
  pref_data.SetBoolean(
      "isBrandedWallpaperNotificationDismissed",
      prefs->GetBoolean(kBrandedWallpaperNotificationDismissed));
  pref_data.SetBoolean("isBraveTodayOptedIn",
                       prefs->GetBoolean(kBraveTodayOptedIn));
  pref_data.SetBoolean(
      "showBinance",
      prefs->GetBoolean(kNewTabPageShowBinance));
  pref_data.SetBoolean(
      "showTogether",
      prefs->GetBoolean(kNewTabPageShowTogether));
  pref_data.SetBoolean(
      "showGemini",
      prefs->GetBoolean(kNewTabPageShowGemini));
#if BUILDFLAG(CRYPTO_DOT_COM_ENABLED)
  pref_data.SetBoolean(
      "showCryptoDotCom",
      prefs->GetBoolean(kCryptoDotComNewTabPageShowCryptoDotCom));
#endif
#if BUILDFLAG(ENABLE_FTX)
  pref_data.SetBoolean("showFTX", prefs->GetBoolean(kFTXNewTabPageShowFTX));
#endif
  return pref_data;
}

base::DictionaryValue GetPrivatePropertiesDictionary(PrefService* prefs) {
  base::DictionaryValue private_data;
  private_data.SetBoolean(
      "useAlternativePrivateSearchEngine",
      prefs->GetBoolean(kUseAlternativeSearchEngineProvider));
  return private_data;
}

base::DictionaryValue GetTorPropertiesDictionary(bool connected,
                                                 const std::string& progress) {
  base::DictionaryValue tor_data;
  tor_data.SetBoolean("torCircuitEstablished", connected);
  tor_data.SetString("torInitProgress", progress);
  return tor_data;
}

// TODO(petemill): Move p3a to own NTP component so it can
// be used by other platforms.

enum class NTPCustomizeUsage {
  kNeverOpened,
  kOpened,
  kOpenedAndEdited,
  kSize
};

const char kNTPCustomizeUsageStatus[] =
    "brave.new_tab_page.customize_p3a_usage";

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

BraveNewTabMessageHandler* BraveNewTabMessageHandler::Create(
      content::WebUIDataSource* source, Profile* profile) {
  //
  // Initial Values
  // Should only contain data that is static
  //
  auto* ads_service_ = brave_ads::AdsServiceFactory::GetForProfile(profile);
  // For safety, default |is_ads_supported_locale_| to true. Better to have
  // false positive than falsen egative,
  // in which case we would not show "opt out" toggle.
  bool is_ads_supported_locale_ = true;
  if (!ads_service_) {
    LOG(ERROR) << "Ads service is not initialized!";
  } else {
    is_ads_supported_locale_ = ads_service_->IsSupportedLocale();
  }

  source->AddBoolean(
      "featureFlagBraveNTPSponsoredImagesWallpaper",
      base::FeatureList::IsEnabled(kBraveNTPBrandedWallpaper) &&
      is_ads_supported_locale_);
  // Private Tab info
  if (IsPrivateNewTab(profile)) {
    source->AddBoolean(
      "isTor", profile->IsTor());
    source->AddBoolean(
      "isQwant", brave::IsRegionForQwant(profile));
  }
  return new BraveNewTabMessageHandler(profile);
}

BraveNewTabMessageHandler::BraveNewTabMessageHandler(Profile* profile)
    : profile_(profile) {
#if BUILDFLAG(ENABLE_TOR)
  tor_launcher_factory_ = TorLauncherFactory::GetInstance();
#endif
}

BraveNewTabMessageHandler::~BraveNewTabMessageHandler() {
#if BUILDFLAG(ENABLE_TOR)
  if (tor_launcher_factory_)
    tor_launcher_factory_->RemoveObserver(this);
#endif
}

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
      "getNewTabPageTorProperties",
      base::BindRepeating(&BraveNewTabMessageHandler::HandleGetTorProperties,
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
      "getBrandedWallpaperData",
      base::BindRepeating(
          &BraveNewTabMessageHandler::HandleGetBrandedWallpaperData,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "customizeClicked",
      base::BindRepeating(&BraveNewTabMessageHandler::HandleCustomizeClicked,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "todayInteractionBegin",
      base::BindRepeating(
          &BraveNewTabMessageHandler::HandleTodayInteractionBegin,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "todayOnCardVisit",
      base::BindRepeating(&BraveNewTabMessageHandler::HandleTodayOnCardVisit,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "todayOnCardViews",
      base::BindRepeating(&BraveNewTabMessageHandler::HandleTodayOnCardViews,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "todayOnPromotedCardView",
      base::BindRepeating(
          &BraveNewTabMessageHandler::HandleTodayOnPromotedCardView,
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
        kUseAlternativeSearchEngineProvider,
        base::BindRepeating(
            &BraveNewTabMessageHandler::OnPrivatePropertiesChanged,
            base::Unretained(this)));
    pref_change_registrar_.Add(
        kAlternativeSearchEngineProviderInTor,
        base::BindRepeating(
            &BraveNewTabMessageHandler::OnPrivatePropertiesChanged,
            base::Unretained(this)));
  }
  // News
  pref_change_registrar_.Add(
      kBraveTodayOptedIn,
      base::Bind(&BraveNewTabMessageHandler::OnPreferencesChanged,
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
      kNewTabPageShowToday,
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
      kNewTabPageShowBinance,
      base::BindRepeating(&BraveNewTabMessageHandler::OnPreferencesChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      kNewTabPageShowTogether,
      base::BindRepeating(&BraveNewTabMessageHandler::OnPreferencesChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      kNewTabPageShowGemini,
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

#if BUILDFLAG(ENABLE_TOR)
  if (tor_launcher_factory_)
    tor_launcher_factory_->AddObserver(this);
#endif
}

void BraveNewTabMessageHandler::OnJavascriptDisallowed() {
  pref_change_registrar_.RemoveAll();
#if BUILDFLAG(ENABLE_TOR)
  if (tor_launcher_factory_)
    tor_launcher_factory_->RemoveObserver(this);
#endif
}

void BraveNewTabMessageHandler::HandleGetPreferences(
        const base::ListValue* args) {
  AllowJavascript();
  PrefService* prefs = profile_->GetPrefs();
  auto data = GetPreferencesDictionary(prefs);
  ResolveJavascriptCallback(args->GetList()[0], data);
}

void BraveNewTabMessageHandler::HandleGetStats(const base::ListValue* args) {
  AllowJavascript();
  PrefService* prefs = profile_->GetPrefs();
  auto data = GetStatsDictionary(prefs);
  ResolveJavascriptCallback(args->GetList()[0], data);
}

void BraveNewTabMessageHandler::HandleGetPrivateProperties(
        const base::ListValue* args) {
  AllowJavascript();
  PrefService* prefs = profile_->GetPrefs();
  auto data = GetPrivatePropertiesDictionary(prefs);
  ResolveJavascriptCallback(args->GetList()[0], data);
}

void BraveNewTabMessageHandler::HandleGetTorProperties(
        const base::ListValue* args) {
  AllowJavascript();
#if BUILDFLAG(ENABLE_TOR)
  auto data = GetTorPropertiesDictionary(
      tor_launcher_factory_ ? tor_launcher_factory_->IsTorConnected() : false,
      "");
#else
  auto data = GetTorPropertiesDictionary(false, "");
#endif
  ResolveJavascriptCallback(args->GetList()[0], data);
}

void BraveNewTabMessageHandler::HandleToggleAlternativeSearchEngineProvider(
    const base::ListValue* args) {
  brave::ToggleUseAlternativeSearchEngineProvider(profile_);
}

void BraveNewTabMessageHandler::HandleSaveNewTabPagePref(
    const base::ListValue* args) {
  if (args->GetSize() != 2) {
    LOG(ERROR) << "Invalid input";
    return;
  }
  brave::RecordValueIfGreater<NTPCustomizeUsage>(
      NTPCustomizeUsage::kOpenedAndEdited, "Brave.NTP.CustomizeUsageStatus",
      kNTPCustomizeUsageStatus, g_browser_process->local_state());
  PrefService* prefs = profile_->GetPrefs();
  // Collect args
  std::string settingsKeyInput = args->GetList()[0].GetString();
  auto settingsValue = args->GetList()[1].Clone();
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
    settingsKey = kNewTabPageShowToday;
  } else if (settingsKeyInput == "isBraveTodayOptedIn") {
    settingsKey = kBraveTodayOptedIn;
  } else if (settingsKeyInput == "showRewards") {
    settingsKey = kNewTabPageShowRewards;
  } else if (settingsKeyInput == "isBrandedWallpaperNotificationDismissed") {
    settingsKey = kBrandedWallpaperNotificationDismissed;
  } else if (settingsKeyInput == "showBinance") {
    settingsKey = kNewTabPageShowBinance;
  } else if (settingsKeyInput == "showTogether") {
    settingsKey = kNewTabPageShowTogether;
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
    const base::ListValue* args) {
  AllowJavascript();

  // Decrement original value only if there's actual branded content
  if (auto* service = ViewCounterServiceFactory::GetForProfile(profile_))
    service->RegisterPageView();
}

void BraveNewTabMessageHandler::HandleBrandedWallpaperLogoClicked(
    const base::ListValue* args) {
  AllowJavascript();
  if (args->GetSize() != 1) {
    LOG(ERROR) << "Invalid input";
    return;
  }

  if (auto* service = ViewCounterServiceFactory::GetForProfile(profile_)) {
    auto* creative_instance_id = args->GetList()[0].FindStringKey(
        ntp_background_images::kCreativeInstanceIDKey);
    auto* destination_url = args->GetList()[0].FindStringPath(
        ntp_background_images::kLogoDestinationURLPath);
    auto* wallpaper_id = args->GetList()[0].FindStringPath(
        ntp_background_images::kWallpaperIDKey);

    DCHECK(creative_instance_id);
    DCHECK(destination_url);
    DCHECK(wallpaper_id);

    service->BrandedWallpaperLogoClicked(
        creative_instance_id ? *creative_instance_id : "",
        destination_url ? *destination_url : "",
        wallpaper_id ? *wallpaper_id : "");
  }
}

void BraveNewTabMessageHandler::HandleGetBrandedWallpaperData(
    const base::ListValue* args) {
  AllowJavascript();

  auto* service = ViewCounterServiceFactory::GetForProfile(profile_);
  auto data = service ? service->GetCurrentWallpaperForDisplay()
                      : base::Value();
  if (!data.is_none()) {
    DCHECK(data.is_dict());

    const std::string wallpaper_id = base::GenerateGUID();
    data.SetStringKey(ntp_background_images::kWallpaperIDKey, wallpaper_id);
    service->BrandedWallpaperWillBeDisplayed(wallpaper_id);
  }

  ResolveJavascriptCallback(args->GetList()[0], std::move(data));
}

void BraveNewTabMessageHandler::HandleCustomizeClicked(
    const base::ListValue* args) {
  AllowJavascript();
  brave::RecordValueIfGreater<NTPCustomizeUsage>(
      NTPCustomizeUsage::kOpened, "Brave.NTP.CustomizeUsageStatus",
      kNTPCustomizeUsageStatus, g_browser_process->local_state());
}

void BraveNewTabMessageHandler::HandleTodayInteractionBegin(
    const base::ListValue* args) {
  AllowJavascript();
  // Track if user has ever scrolled to Brave Today.
  UMA_HISTOGRAM_EXACT_LINEAR("Brave.Today.HasEverInteracted", 1, 1);
  // Track how many times in the past week
  // user has scrolled to Brave Today.
  WeeklyStorage session_count_storage(
      profile_->GetPrefs(), kBraveTodayWeeklySessionCount);
  session_count_storage.AddDelta(1);
  uint64_t total_session_count = session_count_storage.GetWeeklySum();
  constexpr int kSessionCountBuckets[] = {0, 1, 3, 7, 12, 18, 25, 1000};
  const int* it_count =
      std::lower_bound(kSessionCountBuckets, std::end(kSessionCountBuckets),
                      total_session_count);
  int answer = it_count - kSessionCountBuckets;
  UMA_HISTOGRAM_EXACT_LINEAR("Brave.Today.WeeklySessionCount", answer,
                             base::size(kSessionCountBuckets) + 1);
}

void BraveNewTabMessageHandler::HandleTodayOnCardVisit(
    const base::ListValue* args) {
  // Argument should be how many cards visited in this session.
  // We need the front-end to give us this since this class
  // will be destroyed and re-created when the user navigates "back",
  // but the front-end will have access to history state in order to
  // keep a count for the session.
  int cards_visited_total = args->GetList()[0].GetInt();
  // Track how many Brave Today cards have been viewed per session
  // (each NTP / NTP Message Handler is treated as 1 session).
  WeeklyStorage storage(
      profile_->GetPrefs(), kBraveTodayWeeklyCardVisitsCount);
  storage.ReplaceTodaysValueIfGreater(cards_visited_total);
  // Send the session with the highest count of cards viewed.
  uint64_t total = storage.GetHighestValueInWeek();
  constexpr int kBuckets[] = {0, 1, 3, 6, 10, 15, 100};
  const int* it_count =
      std::lower_bound(kBuckets, std::end(kBuckets),
                      total);
  int answer = it_count - kBuckets;
  UMA_HISTOGRAM_EXACT_LINEAR("Brave.Today.WeeklyMaxCardVisitsCount", answer,
                             base::size(kBuckets) + 1);
  // Record ad click if a promoted card was read.
  if (args->GetSize() < 4) {
    return;
  }
  std::string item_id = args->GetList()[1].GetString();
  std::string creative_instance_id = args->GetList()[2].GetString();
  bool is_promoted = args->GetList()[3].GetBool();
  if (is_promoted && !item_id.empty() && !creative_instance_id.empty()) {
    auto* ads_service_ = brave_ads::AdsServiceFactory::GetForProfile(profile_);
    ads_service_->OnPromotedContentAdEvent(
        item_id, creative_instance_id,
        ads::mojom::BraveAdsPromotedContentAdEventType::kClicked);
  }
}

void BraveNewTabMessageHandler::HandleTodayOnCardViews(
    const base::ListValue* args) {
  // Argument should be how many cards viewed in this session.
  int cards_viewed_total = args->GetList()[0].GetInt();
  // Track how many Brave Today cards have been viewed per session
  // (each NTP / NTP Message Handler is treated as 1 session).
  WeeklyStorage storage(
      profile_->GetPrefs(), kBraveTodayWeeklyCardViewsCount);
  storage.ReplaceTodaysValueIfGreater(cards_viewed_total);
  // Send the session with the highest count of cards viewed.
  uint64_t total = storage.GetHighestValueInWeek();
  constexpr int kBuckets[] = {0, 1, 4, 12, 20, 40, 80, 1000};
  const int* it_count =
      std::lower_bound(kBuckets, std::end(kBuckets),
                      total);
  int answer = it_count - kBuckets;
  UMA_HISTOGRAM_EXACT_LINEAR("Brave.Today.WeeklyMaxCardViewsCount", answer,
                             base::size(kBuckets) + 1);
}

void BraveNewTabMessageHandler::HandleTodayOnPromotedCardView(
    const base::ListValue* args) {
  // Argument should be how many cards viewed in this session.
  std::string creative_instance_id = args->GetList()[0].GetString();
  std::string item_id = args->GetList()[1].GetString();
  if (!item_id.empty() && !creative_instance_id.empty()) {
    auto* ads_service_ = brave_ads::AdsServiceFactory::GetForProfile(profile_);
    ads_service_->OnPromotedContentAdEvent(
        item_id, creative_instance_id,
        ads::mojom::BraveAdsPromotedContentAdEventType::kViewed);
  }
}

void BraveNewTabMessageHandler::OnPrivatePropertiesChanged() {
  PrefService* prefs = profile_->GetPrefs();
  auto data = GetPrivatePropertiesDictionary(prefs);
  FireWebUIListener("private-tab-data-updated", data);
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

void BraveNewTabMessageHandler::OnTorCircuitEstablished(bool result) {
  auto data = GetTorPropertiesDictionary(result, "");
  FireWebUIListener("tor-tab-data-updated", data);
}

void BraveNewTabMessageHandler::OnTorInitializing(
    const std::string& percentage) {
  auto data = GetTorPropertiesDictionary(false, percentage);
  FireWebUIListener("tor-tab-data-updated", data);
}
