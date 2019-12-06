// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_new_tab_message_handler.h"

#include <string>
#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/no_destructor.h"
#include "base/values.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/search_engines/search_engine_provider_util.h"
#include "brave/browser/ui/webui/brave_new_tab_ui.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_ui_data_source.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h" //  NOLINT
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"

namespace {

struct BrandedWallpaperLogo {
  std::string imageUrl;
  std::string altText;
  std::string companyName;
  std::string destinationUrl;
};

struct BrandedWallpaper {
  std::string wallpaperImageUrl;
  struct BrandedWallpaperLogo logo;
};

const BrandedWallpaper kDemoWallpaper = {
  "ntp-dummy-brandedwallpaper-background.jpg",
  {
    "ntp-dummy-brandedwallpaper-logo.png",
    "Technikke: For music lovers.",
    "Technikke",
    "https://brave.com"
  }
};

constexpr int kInitialCountToBrandedWallpaper = 1;
constexpr int kRegularCountToBrandedWallpaper = 3;
class NewTabPageViewCounter : public KeyedService {
 public:
  explicit NewTabPageViewCounter(Profile* profile): profile_(profile) {}

  void RegisterPageView() {
    // Don't do any counting if we will never be showing the data
    if (!GetHasBrandedWallpaper()) {
      return;
    }
    this->count_to_branded_wallpaper_--;
    if (this->count_to_branded_wallpaper_ < 0)
      this->count_to_branded_wallpaper_ = kRegularCountToBrandedWallpaper;
  }

  bool GetShouldShowBrandedWallpaper() {
    return GetHasBrandedWallpaper() && (
        this->count_to_branded_wallpaper_ == 0);
  }

  const BrandedWallpaper* GetBrandedWallpaper() {
    // TODO(petemill): lookup flag to choose between demo and real
    return &kDemoWallpaper;
  }

 private:
  bool GetHasBrandedWallpaper() {
    // If user has opted-out, never return data
    if (!profile_->GetPrefs()->
        GetBoolean(kNewTabPageShowBrandedBackgroundImage)) {
      return false;
    }
    // TODO(petemill): lookup flag to always use demo wallpaper
    // or check real data source for actual data.
    return true;
  }

  Profile* profile_;
  int count_to_branded_wallpaper_ = kInitialCountToBrandedWallpaper;

  DISALLOW_COPY_AND_ASSIGN(NewTabPageViewCounter);
};

class NewTabPageViewCounterFactory : public BrowserContextKeyedServiceFactory {
 public:
  // Returns the CaptivePortalService for |profile|.
  static NewTabPageViewCounter* GetForProfile(Profile* profile) {
    return static_cast<NewTabPageViewCounter*>(
    GetInstance()->GetServiceForBrowserContext(profile, true));
  }

  static NewTabPageViewCounterFactory* GetInstance() {
    static base::NoDestructor<NewTabPageViewCounterFactory> instance;
    return instance.get();
  }
  NewTabPageViewCounterFactory() : BrowserContextKeyedServiceFactory(
        "NewTabPageViewCounter",
        BrowserContextDependencyManager::GetInstance()) { }
  ~NewTabPageViewCounterFactory() override { }

 private:
  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* browser_context) const override {
    return new NewTabPageViewCounter(
        Profile::FromBrowserContext(browser_context));
  }
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override {
    return chrome::GetBrowserContextRedirectedInIncognito(context);
  }

  DISALLOW_COPY_AND_ASSIGN(NewTabPageViewCounterFactory);
};

bool IsPrivateNewTab(Profile* profile) {
  return brave::IsTorProfile(profile) || profile->IsIncognitoProfile();
}

base::DictionaryValue GetBrandedWallpaperDictionary(
    const BrandedWallpaper* wallpaper) {
  base::DictionaryValue data;
  data.SetString("wallpaperImageUrl", wallpaper->wallpaperImageUrl);
  auto logo_data = std::make_unique<base::DictionaryValue>();
  logo_data->SetString("image", wallpaper->logo.imageUrl);
  logo_data->SetString("companyName", wallpaper->logo.companyName);
  logo_data->SetString("alt", wallpaper->logo.altText);
  logo_data->SetString("destintationUrl", wallpaper->logo.destinationUrl);
  data.SetDictionary("logo", std::move(logo_data));
  return data;
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
    "httpsUpgradesStat",
    prefs->GetUint64(kHttpsUpgrades));
  stats_data.SetInteger(
    "fingerprintingBlockedStat",
    prefs->GetUint64(kFingerprintingBlocked));
  return stats_data;
}

base::DictionaryValue GetPreferencesDictionary(PrefService* prefs) {
  base::DictionaryValue pref_data;
  pref_data.SetBoolean(
      "showBackgroundImage",
      prefs->GetBoolean(kNewTabPageShowBackgroundImage));
  pref_data.SetBoolean(
      "brandedWallpaperOptIn",
      prefs->GetBoolean(kNewTabPageShowBrandedBackgroundImage));
  pref_data.SetBoolean(
      "showClock",
      prefs->GetBoolean(kNewTabPageShowClock));
  pref_data.SetBoolean(
      "showTopSites",
      prefs->GetBoolean(kNewTabPageShowTopSites));
  pref_data.SetBoolean(
      "showStats",
      prefs->GetBoolean(kNewTabPageShowStats));
  pref_data.SetBoolean(
      "showRewards",
      prefs->GetBoolean(kNewTabPageShowRewards));
  return pref_data;
}

base::DictionaryValue GetPrivatePropertiesDictionary(PrefService* prefs) {
  base::DictionaryValue private_data;
  private_data.SetBoolean(
      "useAlternativePrivateSearchEngine",
      prefs->GetBoolean(kUseAlternativeSearchEngineProvider));
  return private_data;
}

}  // namespace

// static
BraveNewTabMessageHandler* BraveNewTabMessageHandler::Create(
      content::WebUIDataSource* source, Profile* profile) {
  //
  // Initial Values
  // Should only contain data that is static
  //
  // Private Tab info
  if (IsPrivateNewTab(profile)) {
    source->AddBoolean(
      "isTor", brave::IsTorProfile(profile));
    source->AddBoolean(
      "isQwant", brave::IsRegionForQwant(profile));
  }
  return new BraveNewTabMessageHandler(profile);
}

BraveNewTabMessageHandler::BraveNewTabMessageHandler(Profile* profile)
    : profile_(profile) {
}

BraveNewTabMessageHandler::~BraveNewTabMessageHandler() {}

void BraveNewTabMessageHandler::RegisterMessages() {
  // TODO(petemill): This MessageHandler can be split up to
  // individual MessageHandlers for each individual topic area,
  // should other WebUI pages wish to consume the APIs:
  // - Stats
  // - Preferences
  // - PrivatePage properties
  web_ui()->RegisterMessageCallback(
    "getNewTabPagePreferences",
    base::BindRepeating(
      &BraveNewTabMessageHandler::HandleGetPreferences,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
    "getNewTabPageStats",
    base::BindRepeating(
      &BraveNewTabMessageHandler::HandleGetStats,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
    "getNewTabPagePrivateProperties",
    base::BindRepeating(
      &BraveNewTabMessageHandler::HandleGetPrivateProperties,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
    "toggleAlternativePrivateSearchEngine",
    base::BindRepeating(
      &BraveNewTabMessageHandler::HandleToggleAlternativeSearchEngineProvider,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
    "saveNewTabPagePref",
    base::BindRepeating(
      &BraveNewTabMessageHandler::HandleSaveNewTabPagePref,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
    "registerNewTabPageView",
    base::BindRepeating(
      &BraveNewTabMessageHandler::HandleRegisterNewTabPageView,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
    "getBrandedWallpaperData",
    base::BindRepeating(
      &BraveNewTabMessageHandler::HandleGetBrandedWallpaperData,
      base::Unretained(this)));
}

void BraveNewTabMessageHandler::OnJavascriptAllowed() {
  // Observe relevant preferences
  PrefService* prefs = profile_->GetPrefs();
  pref_change_registrar_.Init(prefs);
  // Stats
  pref_change_registrar_.Add(kAdsBlocked,
    base::Bind(&BraveNewTabMessageHandler::OnStatsChanged,
    base::Unretained(this)));
  pref_change_registrar_.Add(kTrackersBlocked,
    base::Bind(&BraveNewTabMessageHandler::OnStatsChanged,
    base::Unretained(this)));
  pref_change_registrar_.Add(kJavascriptBlocked,
    base::Bind(&BraveNewTabMessageHandler::OnStatsChanged,
    base::Unretained(this)));
  pref_change_registrar_.Add(kHttpsUpgrades,
    base::Bind(&BraveNewTabMessageHandler::OnStatsChanged,
    base::Unretained(this)));
  pref_change_registrar_.Add(kFingerprintingBlocked,
    base::Bind(&BraveNewTabMessageHandler::OnStatsChanged,
    base::Unretained(this)));

  if (IsPrivateNewTab(profile_)) {
    // Private New Tab Page preferences
    pref_change_registrar_.Add(kUseAlternativeSearchEngineProvider,
      base::Bind(&BraveNewTabMessageHandler::OnPrivatePropertiesChanged,
      base::Unretained(this)));
    pref_change_registrar_.Add(kAlternativeSearchEngineProviderInTor,
      base::Bind(&BraveNewTabMessageHandler::OnPrivatePropertiesChanged,
      base::Unretained(this)));
  }
  // New Tab Page preferences
  pref_change_registrar_.Add(kNewTabPageShowBackgroundImage,
    base::Bind(&BraveNewTabMessageHandler::OnPreferencesChanged,
    base::Unretained(this)));
  pref_change_registrar_.Add(kNewTabPageShowBrandedBackgroundImage,
    base::Bind(&BraveNewTabMessageHandler::OnPreferencesChanged,
    base::Unretained(this)));
  pref_change_registrar_.Add(kNewTabPageShowClock,
    base::Bind(&BraveNewTabMessageHandler::OnPreferencesChanged,
    base::Unretained(this)));
  pref_change_registrar_.Add(kNewTabPageShowStats,
    base::Bind(&BraveNewTabMessageHandler::OnPreferencesChanged,
    base::Unretained(this)));
  pref_change_registrar_.Add(kNewTabPageShowTopSites,
    base::Bind(&BraveNewTabMessageHandler::OnPreferencesChanged,
    base::Unretained(this)));
  pref_change_registrar_.Add(kNewTabPageShowRewards,
    base::Bind(&BraveNewTabMessageHandler::OnPreferencesChanged,
    base::Unretained(this)));
}

void BraveNewTabMessageHandler::OnJavascriptDisallowed() {
  pref_change_registrar_.RemoveAll();
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
  PrefService* prefs = profile_->GetPrefs();
  // Collect args
  std::string settingsKeyInput = args->GetList()[0].GetString();
  auto settingsValue = args->GetList()[1].Clone();
  // Validate args
  // Note: if we introduce any non-bool settings values
  // then perform this type check within the appropriate key conditionals.
  if (!settingsValue.is_bool()) {
    LOG(ERROR) << "Invalid value type";
    return;
  }
  const auto settingsValueBool = settingsValue.GetBool();
  std::string settingsKey;
  if (settingsKeyInput == "showBackgroundImage") {
    settingsKey = kNewTabPageShowBackgroundImage;
  } else if (settingsKeyInput == "brandedWallpaperOptIn") {
    settingsKey = kNewTabPageShowBrandedBackgroundImage;
  } else if (settingsKeyInput == "showClock") {
    settingsKey = kNewTabPageShowClock;
  } else if (settingsKeyInput == "showTopSites") {
    settingsKey = kNewTabPageShowTopSites;
  } else if (settingsKeyInput == "showStats") {
    settingsKey = kNewTabPageShowStats;
  } else if (settingsKeyInput == "showRewards") {
    settingsKey = kNewTabPageShowRewards;
  } else {
    LOG(ERROR) << "Invalid setting key";
    return;
  }
  prefs->SetBoolean(settingsKey, settingsValueBool);
}

void BraveNewTabMessageHandler::HandleRegisterNewTabPageView(
    const base::ListValue* args) {
  AllowJavascript();
  // Decrement original value only if there's actual branded content

  NewTabPageViewCounterFactory::GetForProfile(profile_)->RegisterPageView();
}

void BraveNewTabMessageHandler::HandleGetBrandedWallpaperData(
    const base::ListValue* args) {
  AllowJavascript();

  auto* service = NewTabPageViewCounterFactory::GetForProfile(profile_);
  if (!service->GetShouldShowBrandedWallpaper()) {
    ResolveJavascriptCallback(args->GetList()[0], base::Value());
    return;
  }
  auto data = GetBrandedWallpaperDictionary(
      service->GetBrandedWallpaper());
  ResolveJavascriptCallback(args->GetList()[0], data);
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
