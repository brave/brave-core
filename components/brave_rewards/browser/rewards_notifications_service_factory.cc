/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/rewards_notifications_service_factory.h"

#include "brave/components/brave_rewards/browser/rewards_notifications_service.h"
#include "brave/components/brave_rewards/browser/buildflags/buildflags.h"
#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
#include "brave/components/brave_rewards/browser/rewards_notifications_service_impl.h"
#endif

namespace brave_rewards {

// static
RewardsNotificationsService* RewardsNotificationsServiceFactory::GetForProfile(Profile* profile) {
  if (profile->IsOffTheRecord())
    return nullptr;
  return static_cast<RewardsNotificationsService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
RewardsNotificationsServiceFactory* RewardsNotificationsServiceFactory::GetInstance() {
  return base::Singleton<RewardsNotificationsServiceFactory>::get();
}

RewardsNotificationsServiceFactory::RewardsNotificationsServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "RewardsNotificationService",
          BrowserContextDependencyManager::GetInstance()) {
}

RewardsNotificationsServiceFactory::~RewardsNotificationsServiceFactory() {
}

KeyedService* RewardsNotificationsServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
#if BUILDFLAG(BRAVE_REWARDS_ENABLED)
  std::unique_ptr<RewardsNotificationsServiceImpl> rewards_notifications_service(
      new RewardsNotificationsServiceImpl(Profile::FromBrowserContext(context)));
  rewards_notifications_service->Init();
  return rewards_notifications_service.release();
#else
  return nullptr;
#endif
}

content::BrowserContext* RewardsNotificationsServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  if (context->IsOffTheRecord())
    return chrome::GetBrowserContextOwnInstanceInIncognito(context);

  // Use original profile for session profiles
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

bool RewardsNotificationsServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

}  // namespace brave_rewards
