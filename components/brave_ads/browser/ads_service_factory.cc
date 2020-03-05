/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "brave/components/brave_ads/browser/ads_service_factory.h"

#include "base/time/time.h"
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

bool AdsServiceFactory::ServiceIsNULLWhileTesting() const {
  return false;
}

void AdsServiceFactory::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterIntegerPref(prefs::kVersion, prefs::kCurrentVersionNumber);

  registry->RegisterIntegerPref(prefs::kSupportedRegionsLastSchemaVersion, 0);

  registry->RegisterIntegerPref(prefs::kSupportedRegionsSchemaVersion,
      prefs::kSupportedRegionsSchemaVersionNumber);

  registry->RegisterBooleanPref(prefs::kEnabled, false);

  registry->RegisterBooleanPref(
      prefs::kShouldShowPublisherAdsOnParticipatingSites, true);

  registry->RegisterBooleanPref(prefs::kShouldAllowAdConversionTracking, true);

  registry->RegisterUint64Pref(prefs::kAdsPerHour, 2);
  registry->RegisterUint64Pref(prefs::kAdsPerDay, 20);

  registry->RegisterIntegerPref(prefs::kIdleThreshold, 15);
  registry->RegisterBooleanPref(prefs::kAdsWereDisabled, false);
  registry->RegisterBooleanPref(prefs::kHasAdsP3AState, false);

  registry->RegisterBooleanPref(prefs::kShouldShowMyFirstAdNotification, true);

  registry->RegisterBooleanPref(prefs::kShouldShowOnboarding, true);
  registry->RegisterUint64Pref(prefs::kOnboardingTimestamp, 0);
}

}  // namespace brave_ads
