// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include <memory>

#include "base/bind.h"
#include "base/no_destructor.h"
#include "brave/browser/ui/webui/new_tab_page/new_tab_page_branded_view_counter.h" //  NOLINT
#include "brave/common/pref_names.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_features.h"
#include "content/public/browser/browser_context.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h" //  NOLINT
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"

namespace {

constexpr int kInitialCountToBrandedWallpaper = 1;
constexpr int kRegularCountToBrandedWallpaper = 3;

std::unique_ptr<BrandedWallpaper> GetDemoWallpaper() {
  auto demo = std::make_unique<BrandedWallpaper>();
  demo->wallpaperImageUrls = {
      "ntp-dummy-brandedwallpaper-background-1.jpg",
      "ntp-dummy-brandedwallpaper-background-2.jpg",
      "ntp-dummy-brandedwallpaper-background-3.jpg"};
  demo->logo = std::make_unique<BrandedWallpaperLogo>();
  demo->logo->imageUrl = "ntp-dummy-brandedwallpaper-logo.png";
  demo->logo->altText = "Technikke: For music lovers.";
  demo->logo->companyName = "Technikke";
  demo->logo->destinationUrl = "https://brave.com";
  return demo;
}

class NewTabPageBrandedViewCounterFactory
    : public BrowserContextKeyedServiceFactory {
 public:
  // Returns the CaptivePortalService for |profile|.
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
        BrowserContextDependencyManager::GetInstance()) { }
  ~NewTabPageBrandedViewCounterFactory() override { }

 private:
  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* browser_context) const override {
    return new NewTabPageBrandedViewCounter(
        Profile::FromBrowserContext(browser_context));
  }
  content::BrowserContext* GetBrowserContextToUse(
      content::BrowserContext* context) const override {
    return chrome::GetBrowserContextRedirectedInIncognito(context);
  }

  DISALLOW_COPY_AND_ASSIGN(NewTabPageBrandedViewCounterFactory);
};

}  // namespace

// static
NewTabPageBrandedViewCounter* NewTabPageBrandedViewCounter::GetForProfile(
    Profile* profile) {
  return NewTabPageBrandedViewCounterFactory::GetForProfile(profile);
}

NewTabPageBrandedViewCounter::NewTabPageBrandedViewCounter(Profile* profile)
    : count_to_branded_wallpaper_(kInitialCountToBrandedWallpaper),
      profile_(profile) {
  // If we have a wallpaper, store it as private var.
  // TODO(petemill): Update the private var when the data source gets
  // new content, when we have a data source!
  // Set demo wallpaper if a flag is set.
  if (base::FeatureList::IsEnabled(features::kBraveNTPBrandedWallpaper) &&
      base::FeatureList::IsEnabled(features::kBraveNTPBrandedWallpaperDemo)) {
    current_wallpaper_ = GetDemoWallpaper();
  }
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
}

NewTabPageBrandedViewCounter::~NewTabPageBrandedViewCounter() { }

void NewTabPageBrandedViewCounter::RegisterPageView() {
  // Don't do any counting if we will never be showing the data
  // since we want the count to start at the point of data being available
  // or the user opt-in status changing.
  if (!IsBrandedWallpaperActive()) {
    return;
  }
  this->count_to_branded_wallpaper_--;
  if (this->count_to_branded_wallpaper_ < 0) {
    this->count_to_branded_wallpaper_ = kRegularCountToBrandedWallpaper;
  } else if (this->count_to_branded_wallpaper_ == 0) {
    // When count is `0` then UI is free to show
    // the branded wallpaper, until the next time `RegisterPageView`
    // is called.
    // We select the appropriate image index for the scheduled
    // view of the branded wallpaper.
    current_wallpaper_image_index_++;
    size_t last_index = current_wallpaper_->wallpaperImageUrls.size() - 1;
    if (current_wallpaper_image_index_ > last_index) {
      current_wallpaper_image_index_ = 0;
    }
  }
}

bool NewTabPageBrandedViewCounter::IsBrandedWallpaperActive() {
  if (!base::FeatureList::IsEnabled(features::kBraveNTPBrandedWallpaper)) {
    return false;
  }
  return (current_wallpaper_ != nullptr && has_user_opted_in_ == true);
}

bool NewTabPageBrandedViewCounter::ShouldShowBrandedWallpaper() {
  return IsBrandedWallpaperActive() && (
      this->count_to_branded_wallpaper_ == 0);
}

const BrandedWallpaper& NewTabPageBrandedViewCounter::GetBrandedWallpaper() {
  return *current_wallpaper_;
}

size_t NewTabPageBrandedViewCounter::GetWallpaperImageIndexToDisplay() {
  return current_wallpaper_image_index_;
}

void NewTabPageBrandedViewCounter::SetShouldShowFromPreferences() {
  auto* prefs = profile_->GetPrefs();
  has_user_opted_in_ = (
      prefs->GetBoolean(kNewTabPageShowBrandedBackgroundImage) &&
      prefs->GetBoolean(kNewTabPageShowBackgroundImage));
}
