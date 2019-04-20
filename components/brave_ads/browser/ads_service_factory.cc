/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/ads_service_factory.h"

#include "brave/components/brave_ads/browser/buildflags/buildflags.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_store.h"

#if BUILDFLAG(BRAVE_ADS_ENABLED)
#include "brave/components/brave_ads/browser/ads_service_impl.h"
#include "chrome/browser/dom_distiller/dom_distiller_service_factory.h"
#include "chrome/browser/notifications/notification_display_service_factory.h"
#include "brave/components/brave_rewards/browser/rewards_service_factory.h"
#endif

class PrefStore;

namespace brave_ads {

// static
AdsService* AdsServiceFactory::GetForProfile(
    Profile* profile) {
  if (profile->IsOffTheRecord())
    return NULL;

  return static_cast<AdsService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
AdsServiceFactory* AdsServiceFactory::GetInstance() {
  return base::Singleton<AdsServiceFactory>::get();
}

AdsServiceFactory::AdsServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "AdsService",
          BrowserContextDependencyManager::GetInstance()) {
#if BUILDFLAG(BRAVE_ADS_ENABLED)
  DependsOn(NotificationDisplayServiceFactory::GetInstance());
  DependsOn(dom_distiller::DomDistillerServiceFactory::GetInstance());
  DependsOn(brave_rewards::RewardsServiceFactory::GetInstance());
#endif
}

AdsServiceFactory::~AdsServiceFactory() {
}

KeyedService* AdsServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
#if BUILDFLAG(BRAVE_ADS_ENABLED)
  std::unique_ptr<AdsServiceImpl> ads_service(
      new AdsServiceImpl(Profile::FromBrowserContext(context)));
  return ads_service.release();
#else
  return NULL;
#endif
}

content::BrowserContext* AdsServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  if (context->IsOffTheRecord())
    return chrome::GetBrowserContextOwnInstanceInIncognito(context);

  // use original profile for session profiles
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

bool AdsServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

void AdsServiceFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  auto should_migrate_prefs_from_62 = ShouldMigratePrefsFrom62(registry);

  if (ShouldMigratePrefs(registry)) {
    // prefs::kBraveAdsPrefsVersion should default to 1 for legacy installations
    // so that preferences are migrated from version 1 to the current version
    registry->RegisterIntegerPref(prefs::kBraveAdsPrefsVersion,
        prefs::kBraveAdsPrefsDefaultVersion);
  } else {
    registry->RegisterIntegerPref(prefs::kBraveAdsPrefsVersion,
        prefs::kBraveAdsPrefsCurrentVersion);
  }

  registry->RegisterBooleanPref(prefs::kBraveAdsEnabled, false);

  registry->RegisterUint64Pref(prefs::kBraveAdsPerHour, 2);

  #if defined(OS_ANDROID)
    registry->RegisterUint64Pref(prefs::kBraveAdsPerDay, 12);
  #else
    registry->RegisterUint64Pref(prefs::kBraveAdsPerDay, 20);
  #endif

  registry->RegisterIntegerPref(prefs::kBraveAdsIdleThreshold, 15);

  if (should_migrate_prefs_from_62) {
    registry->RegisterBooleanPref(prefs::kBraveAdsPrefsMigratedFrom62, true);
  }
}

bool AdsServiceFactory::ShouldMigratePrefs(
    user_prefs::PrefRegistrySyncable* registry) const {
  // If prefs::kBraveAdsEnabled does not exist then this must be a fresh
  // installion so we do not need to migrate
  auto pref_store = registry->defaults();

  const base::Value* value = nullptr;
  if (!pref_store->GetValue(prefs::kBraveAdsEnabled, &value)) {
    return false;
  }

  return true;
}

bool AdsServiceFactory::ShouldMigratePrefsFrom62(
    user_prefs::PrefRegistrySyncable* registry) const {
  // prefs::kBraveAdsPrefsVersion has existed since 0.63.45 so if this key does
  // not exist then this must be an upgrade from 0.62.x so we should migrate
  auto pref_store = registry->defaults();

  const base::Value* value = nullptr;
  if (pref_store->GetValue(prefs::kBraveAdsPrefsVersion, &value)) {
    return false;
  }

  return true;
}

}  // namespace brave_ads
