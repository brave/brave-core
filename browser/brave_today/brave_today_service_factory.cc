// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/brave_today/brave_today_service_factory.h"

#include <memory>

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/browser/ads_service_factory.h"
#include "brave/components/brave_today/browser/brave_today_service.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_context.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"

namespace brave_today {

// static
BraveTodayService* BraveTodayServiceFactory::GetForProfile(Profile* profile) {
  return static_cast<BraveTodayService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
BraveTodayServiceFactory* BraveTodayServiceFactory::GetInstance() {
  return base::Singleton<BraveTodayServiceFactory>::get();
}

BraveTodayServiceFactory::BraveTodayServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "BraveTodayService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(brave_ads::AdsServiceFactory::GetInstance());
}

BraveTodayServiceFactory::~BraveTodayServiceFactory() {}

KeyedService* BraveTodayServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* browser_context) const {
  // Only NTP in normal profile uses Brave Today.
  if (!brave::IsRegularProfile(browser_context))
    return nullptr;

  Profile* profile = Profile::FromBrowserContext(browser_context);
  auto* ads_service = brave_ads::AdsServiceFactory::GetForProfile(profile);

  return new BraveTodayService(ads_service,
                                profile->GetPrefs(),
                                g_brave_browser_process->local_state());
}

void BraveTodayServiceFactory::RegisterProfilePrefs(
     user_prefs::PrefRegistrySyncable* registry) {
  BraveTodayService::RegisterProfilePrefs(registry);
}

bool BraveTodayServiceFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}

}  // namespace brave_today
