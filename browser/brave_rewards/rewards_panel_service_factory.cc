/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_rewards/rewards_panel_service_factory.h"

#include "brave/browser/brave_rewards/rewards_panel_service.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/profiles/profile_util.h"
#include "chrome/browser/profiles/profile.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"

namespace brave_rewards {

// static
RewardsPanelService* RewardsPanelServiceFactory::GetForProfile(
    Profile* profile) {
  return static_cast<RewardsPanelService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
RewardsPanelServiceFactory* RewardsPanelServiceFactory::GetInstance() {
  static base::NoDestructor<RewardsPanelServiceFactory> factory;
  return factory.get();
}

RewardsPanelServiceFactory::RewardsPanelServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "RewardsPanelService",
          BrowserContextDependencyManager::GetInstance()) {}

KeyedService* RewardsPanelServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new RewardsPanelService(Profile::FromBrowserContext(context));
}

content::BrowserContext* RewardsPanelServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return RewardsServiceFactory::IsServiceAllowedForContext(context) ? context
                                                                    : nullptr;
}

RewardsPanelServiceFactory::~RewardsPanelServiceFactory() = default;

}  // namespace brave_rewards
