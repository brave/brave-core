// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ntp_sponsored_images/view_counter_service_factory.h"

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/browser/ads_service_factory.h"
#include "brave/components/ntp_sponsored_images/browser/features.h"
#include "brave/components/ntp_sponsored_images/browser/ntp_sponsored_images_service.h"
#include "brave/components/ntp_sponsored_images/browser/view_counter_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_features.h"
#include "content/public/browser/browser_context.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"

namespace ntp_sponsored_images {

// static
ViewCounterService* ViewCounterServiceFactory::GetForProfile(Profile* profile) {
  if (base::FeatureList::IsEnabled(features::kBraveNTPBrandedWallpaper))
    return static_cast<ViewCounterService*>(
        GetInstance()->GetServiceForBrowserContext(profile, true));

  return nullptr;
}

// static
ViewCounterServiceFactory* ViewCounterServiceFactory::GetInstance() {
  return base::Singleton<ViewCounterServiceFactory>::get();
}

ViewCounterServiceFactory::ViewCounterServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "ViewCounterService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(brave_ads::AdsServiceFactory::GetInstance());
}

ViewCounterServiceFactory::~ViewCounterServiceFactory() {}

KeyedService* ViewCounterServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* browser_context) const {
  Profile* profile = Profile::FromBrowserContext(browser_context);

  bool is_supported_locale = false;
  auto* ads_service = brave_ads::AdsServiceFactory::GetForProfile(profile);
  if (!ads_service) {
    LOG(ERROR) << "Ads service was disabled at build time!";
  } else {
    is_supported_locale = ads_service->IsSupportedLocale();
  }

  auto* service = g_brave_browser_process->ntp_sponsored_images_service();
  service->AddDataSource(browser_context);

  ViewCounterService* instance = new ViewCounterService(
      service,
      profile->GetPrefs(),
      is_supported_locale);
  return instance;
}

void ViewCounterServiceFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(
      kBrandedWallpaperNotificationDismissed, false);
  registry->RegisterBooleanPref(
      kNewTabPageShowBrandedBackgroundImage, true);
}

bool ViewCounterServiceFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}

}  // namespace ntp_sponsored_images
