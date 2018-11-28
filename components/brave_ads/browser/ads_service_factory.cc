/* This Source Code Form is subject to the terms of the Mozilla Public
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

#if BUILDFLAG(BRAVE_ADS_ENABLED)
#include "brave/components/brave_ads/browser/ads_service_impl.h"
#include "chrome/browser/dom_distiller/dom_distiller_service_factory.h"
#include "chrome/browser/notifications/notification_display_service_factory.h"
#endif

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
  registry->RegisterBooleanPref(prefs::kBraveAdsEnabled, false);
  registry->RegisterUint64Pref(prefs::kBraveAdsPerHour, 2);
  registry->RegisterUint64Pref(prefs::kBraveAdsPerDay, 6);
  registry->RegisterIntegerPref(prefs::kBraveAdsIdleThreshold, 15);
}

}  // namespace brave_ads
