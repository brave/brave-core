// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include <memory>
#include <string>

#include "base/bind.h"
#include "base/no_destructor.h"
#include "base/strings/stringprintf.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/ntp_sponsored_images/new_tab_page_branded_view_counter.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/browser/ads_service_factory.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/browser/rewards_service_factory.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_features.h"
#include "content/public/browser/browser_context.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"

namespace {

bool IsPrivateNewTab(Profile* profile) {
  return brave::IsTorProfile(profile) || profile->IsIncognitoProfile();
}

std::unique_ptr<NTPSponsoredImagesData> GetDemoWallpaper() {
  auto demo = std::make_unique<NTPSponsoredImagesData>();
  demo->wallpaper_image_urls = {
      "ntp-dummy-brandedwallpaper-background-1.jpg",
      "ntp-dummy-brandedwallpaper-background-2.jpg",
      "ntp-dummy-brandedwallpaper-background-3.jpg"};
  demo->logo_image_url = "ntp-dummy-brandedwallpaper-logo.png";
  demo->logo_alt_text = "Technikke: For music lovers.";
  demo->logo_company_name = "Technikke";
  demo->logo_destination_url = "https://brave.com";
  return demo;
}

class NewTabPageBrandedViewCounterFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  static NewTabPageBrandedViewCounter* GetForProfile(Profile* profile) {
    return static_cast<NewTabPageBrandedViewCounter*>(
        GetInstance()->GetServiceForBrowserContext(profile, true));
  }

  static NewTabPageBrandedViewCounterFactory* GetInstance() {
    static base::NoDestructor<NewTabPageBrandedViewCounterFactory> instance;
    return instance.get();
  }

  NewTabPageBrandedViewCounterFactory() : BrowserContextKeyedServiceFactory(
        "NewTabPageBrandedViewCounter",
        BrowserContextDependencyManager::GetInstance()) {
    DependsOn(brave_rewards::RewardsServiceFactory::GetInstance());
    DependsOn(brave_ads::AdsServiceFactory::GetInstance());
  }
  ~NewTabPageBrandedViewCounterFactory() override = default;

  NewTabPageBrandedViewCounterFactory(
      const NewTabPageBrandedViewCounterFactory&) = delete;
  NewTabPageBrandedViewCounterFactory& operator=(
      const NewTabPageBrandedViewCounterFactory&) = delete;

 private:
  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* browser_context) const override {
    Profile* profile = Profile::FromBrowserContext(browser_context);
    DCHECK(!IsPrivateNewTab(profile));
    NewTabPageBrandedViewCounter* instance = new NewTabPageBrandedViewCounter(
        profile);
    return instance;
  }
  void RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) override {
    registry->RegisterBooleanPref(
        kBrandedWallpaperNotificationDismissed, false);
    registry->RegisterBooleanPref(
        kNewTabPageShowBrandedBackgroundImage, true);
  }
  bool ServiceIsCreatedWithBrowserContext() const override {
    return true;
  }
};

}  // namespace

// static
void  NewTabPageBrandedViewCounter::
    EnsureBrowserContextKeyedServiceFactoriesBuilt() {
  NewTabPageBrandedViewCounterFactory::GetInstance();
}

NewTabPageBrandedViewCounter* NewTabPageBrandedViewCounter::GetForProfile(
    Profile* profile) {
  if (base::FeatureList::IsEnabled(features::kBraveNTPBrandedWallpaper))
    return NewTabPageBrandedViewCounterFactory::GetForProfile(profile);
  return nullptr;
}

NewTabPageBrandedViewCounter::NewTabPageBrandedViewCounter(Profile* profile)
    : profile_(profile) {
  // If we have a wallpaper, store it as private var.
  // Set demo wallpaper if a flag is set.
  if (base::FeatureList::IsEnabled(features::kBraveNTPBrandedWallpaperDemo)) {
    current_wallpaper_ = GetDemoWallpaper();
  } else {
    manager_ =
        g_brave_browser_process->ntp_sponsored_images_component_manager();
    manager_->AddObserver(this);
    manager_->AddDataSource(profile_);
    // Check if we have real data
    const auto optional_data = manager_->GetLatestSponsoredImagesData();
    if (optional_data) {
      current_wallpaper_.reset(new NTPSponsoredImagesData(*optional_data));
    }
  }
  if (current_wallpaper_) {
    model_.set_total_image_count(
        current_wallpaper_->wallpaper_image_urls.size());
  }
  // Allow notification dismissal pref to be reset when rewards status changes.
  auto* rewards_service_ =
      brave_rewards::RewardsServiceFactory::GetForProfile(profile);
  if (rewards_service_)
    rewards_service_->AddObserver(this);
  // Observe relevant preferences that affect whether we should show
  // wallpaper or count views.
  SetShouldShowFromPreferences();
  PrefService* prefs = profile->GetPrefs();
  pref_change_registrar_.Init(prefs);
  pref_change_registrar_.Add(kNewTabPageShowBackgroundImage,
      base::Bind(&NewTabPageBrandedViewCounter::SetShouldShowFromPreferences,
      base::Unretained(this)));
  pref_change_registrar_.Add(kNewTabPageShowBrandedBackgroundImage,
      base::Bind(&NewTabPageBrandedViewCounter::SetShouldShowFromPreferences,
      base::Unretained(this)));
  auto* ads_service_ = brave_ads::AdsServiceFactory::GetForProfile(profile);
  if (!ads_service_) {
    LOG(ERROR) << "Ads service is not initialized!";
  } else {
    is_supported_locale_ = ads_service_->IsSupportedLocale();
  }
}

NewTabPageBrandedViewCounter::~NewTabPageBrandedViewCounter() = default;

void NewTabPageBrandedViewCounter::Shutdown() {
  if (manager_ && manager_->HasObserver(this))
    manager_->RemoveObserver(this);
  auto* rewards_service_ =
      brave_rewards::RewardsServiceFactory::GetForProfile(profile_);
  if (rewards_service_)
    rewards_service_->RemoveObserver(this);
}

void NewTabPageBrandedViewCounter::OnUpdated(
    const NTPSponsoredImagesData& data) {
  DCHECK(
      !base::FeatureList::IsEnabled(features::kBraveNTPBrandedWallpaperDemo));
  DCHECK(manager_);

  // Data is updated, so change our stored data and reset any indexes.
  // But keep view counter until branded content is seen.
  model_.ResetCurrentWallpaperImageIndex();
  model_.set_total_image_count(data.wallpaper_image_urls.size());
  current_wallpaper_.reset(new NTPSponsoredImagesData(data));
  manager_->AddDataSource(profile_);
}

void NewTabPageBrandedViewCounter::OnRewardsMainEnabled(
    brave_rewards::RewardsService* rewards_service, bool rewards_main_enabled) {
  // Reset notification state
  ResetNotificationState();
}

void NewTabPageBrandedViewCounter::OnAdsEnabled(
    brave_rewards::RewardsService* rewards_service, bool ads_enabled) {
  // Reset notification state
  ResetNotificationState();
}

void NewTabPageBrandedViewCounter::ResetNotificationState() {
  auto* prefs = profile_->GetPrefs();
  prefs->SetBoolean(kBrandedWallpaperNotificationDismissed, false);
}

void NewTabPageBrandedViewCounter::RegisterPageView() {
  // Don't do any counting if we will never be showing the data
  // since we want the count to start at the point of data being available
  // or the user opt-in status changing.
  if (IsBrandedWallpaperActive()) {
    model_.RegisterPageView();
  }
}

bool NewTabPageBrandedViewCounter::IsBrandedWallpaperActive() {
  return (is_supported_locale_ && has_user_opted_in_ &&
     current_wallpaper_ != nullptr);
}

bool NewTabPageBrandedViewCounter::ShouldShowBrandedWallpaper() {
  return IsBrandedWallpaperActive() &&
      model_.ShouldShowBrandedWallpaper();
}

const
NTPSponsoredImagesData& NewTabPageBrandedViewCounter::GetBrandedWallpaper() {
  return *current_wallpaper_;
}

size_t NewTabPageBrandedViewCounter::GetWallpaperImageIndexToDisplay() {
  return model_.current_wallpaper_image_index();
}

void NewTabPageBrandedViewCounter::SetShouldShowFromPreferences() {
  auto* prefs = profile_->GetPrefs();
  has_user_opted_in_ = (
      prefs->GetBoolean(kNewTabPageShowBrandedBackgroundImage) &&
      prefs->GetBoolean(kNewTabPageShowBackgroundImage));
}
