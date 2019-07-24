/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "brave/components/brave_rewards/browser/rewards_service_factory.h"

#include "brave/browser/profiles/brave_profile_manager.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_rewards/browser/buildflags/buildflags.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "extensions/browser/event_router_factory.h"
#endif

#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#endif

namespace {
#if BUILDFLAG(BRAVE_REWARDS_ENABLED) && !defined(OS_ANDROID)
void OverridePrefsForPrivateProfileUserPrefs(Profile* profile) {
  if (profile->IsRegularProfile())
    return;

  // rewards button should be hidden on guest and tor profile.
  PrefService* pref_service = profile->GetPrefs();
  pref_service->SetBoolean(kHideBraveRewardsButton, true);
  pref_service->SetBoolean(brave_rewards::prefs::kBraveRewardsEnabled, false);
}
#endif
}  // namespace

namespace brave_rewards {

RewardsService* testing_service_;

// static
RewardsService* RewardsServiceFactory::GetForProfile(
    Profile* profile) {
  if (profile->IsOffTheRecord())
    return NULL;

  if (testing_service_) {
    return testing_service_;
  }

  return static_cast<RewardsService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
RewardsServiceFactory* RewardsServiceFactory::GetInstance() {
  return base::Singleton<RewardsServiceFactory>::get();
}

RewardsServiceFactory::RewardsServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "RewardsService",
          BrowserContextDependencyManager::GetInstance()) {
#if BUILDFLAG(ENABLE_EXTENSIONS)
  DependsOn(extensions::EventRouterFactory::GetInstance());
#endif

#if BUILDFLAG(BRAVE_REWARDS_ENABLED) && !defined(OS_ANDROID)
  registrar_.Add(this, chrome::NOTIFICATION_PROFILE_CREATED,
                 content::NotificationService::AllSources());
#endif
}

RewardsServiceFactory::~RewardsServiceFactory() {
}

KeyedService* RewardsServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
  std::unique_ptr<RewardsServiceImpl> rewards_service(
      new RewardsServiceImpl(Profile::FromBrowserContext(context)));
  rewards_service->Init();
  return rewards_service.release();
#else
  return NULL;
#endif
}

// static
void RewardsServiceFactory::SetServiceForTesting(RewardsService* service) {
  testing_service_ = service;
}

content::BrowserContext* RewardsServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  if (context->IsOffTheRecord())
    return chrome::GetBrowserContextOwnInstanceInIncognito(context);

  // use original profile for session profiles
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

bool RewardsServiceFactory::ServiceIsNULLWhileTesting() const {
  return false;
}

void RewardsServiceFactory::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
#if BUILDFLAG(BRAVE_REWARDS_ENABLED) && !defined(OS_ANDROID)
  switch (type) {
    case chrome::NOTIFICATION_PROFILE_CREATED: {
      auto* profile = content::Source<Profile>(source).ptr();
      OverridePrefsForPrivateProfileUserPrefs(profile);
      break;
    }
    default:
      NOTREACHED();
  }
#endif
}

}  // namespace brave_rewards
