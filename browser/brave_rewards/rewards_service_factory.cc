/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>

#include "brave/browser/brave_rewards/rewards_service_factory.h"

#include "brave/browser/profiles/brave_profile_manager.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/components/brave_rewards/browser/buildflags/buildflags.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service_observer.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/browser/rewards_service_private_observer.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/greaselion/browser/buildflags/buildflags.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "extensions/browser/event_router_factory.h"
#include "brave/browser/brave_rewards/extension_rewards_service_observer.h"
#include "brave/browser/brave_rewards/extension_rewards_notification_service_observer.h"
#endif

#if BUILDFLAG(ENABLE_GREASELION)
#include "brave/browser/greaselion/greaselion_service_factory.h"
#include "brave/components/greaselion/browser/greaselion_service.h"
#endif

#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
#include "brave/components/brave_rewards/browser/rewards_service_impl.h"
#endif

namespace {
#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
void OverridePrefsForPrivateProfileUserPrefs(Profile* profile) {
  if (brave::IsRegularProfile(profile))
    return;

  // rewards button should be hidden on guest and tor profile.
  PrefService* pref_service = profile->GetPrefs();
  pref_service->SetBoolean(brave_rewards::prefs::kHideButton, true);
}
#endif
}  // namespace

namespace brave_rewards {

RewardsService* testing_service_;

// static
RewardsService* RewardsServiceFactory::GetForProfile(
    Profile* profile) {
  if (testing_service_) {
    return testing_service_;
  }

  if (!brave::IsRegularProfile(profile)) {
    return nullptr;
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
#if BUILDFLAG(ENABLE_GREASELION)
  DependsOn(greaselion::GreaselionServiceFactory::GetInstance());
#endif

#if BUILDFLAG(BRAVE_REWARDS_ENABLED) && !defined(OS_ANDROID)
  if (g_browser_process && g_browser_process->profile_manager())
    g_browser_process->profile_manager()->AddObserver(this);
#endif
}

RewardsServiceFactory::~RewardsServiceFactory() {
#if BUILDFLAG(BRAVE_REWARDS_ENABLED) && !defined(OS_ANDROID)
  if (g_browser_process && g_browser_process->profile_manager())
    g_browser_process->profile_manager()->RemoveObserver(this);
#endif
}

KeyedService* RewardsServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
  std::unique_ptr<RewardsServiceObserver> extension_observer = nullptr;
  std::unique_ptr<RewardsServicePrivateObserver> private_observer = nullptr;
  std::unique_ptr<RewardsNotificationServiceObserver> notification_observer =
      nullptr;
  #if BUILDFLAG(ENABLE_EXTENSIONS)
    extension_observer = std::make_unique<ExtensionRewardsServiceObserver>(
          Profile::FromBrowserContext(context));
    private_observer = std::make_unique<ExtensionRewardsServiceObserver>(
          Profile::FromBrowserContext(context));
    notification_observer =
        std::make_unique<ExtensionRewardsNotificationServiceObserver>(
          Profile::FromBrowserContext(context));
  #endif
  #if BUILDFLAG(ENABLE_GREASELION)
    greaselion::GreaselionService* greaselion_service =
        greaselion::GreaselionServiceFactory::GetForBrowserContext(context);
    std::unique_ptr<RewardsServiceImpl> rewards_service(new RewardsServiceImpl(
        Profile::FromBrowserContext(context), greaselion_service));
#else
    std::unique_ptr<RewardsServiceImpl> rewards_service(
        new RewardsServiceImpl(Profile::FromBrowserContext(context)));
  #endif
  rewards_service->Init(std::move(extension_observer),
                        std::move(private_observer),
                        std::move(notification_observer));
  return rewards_service.release();
#else
  return nullptr;
#endif
}

// static
void RewardsServiceFactory::SetServiceForTesting(RewardsService* service) {
  testing_service_ = service;
}

bool RewardsServiceFactory::ServiceIsNULLWhileTesting() const {
  return false;
}

void RewardsServiceFactory::OnProfileAdded(Profile* profile) {
#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
  OverridePrefsForPrivateProfileUserPrefs(profile);
#endif
}

}  // namespace brave_rewards
