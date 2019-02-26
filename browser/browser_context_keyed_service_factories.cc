/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/browser_context_keyed_service_factories.h"

#include "brave/browser/search_engines/search_engine_provider_service_factory.h"
#include "brave/browser/tor/tor_profile_service_factory.h"
#include "brave/components/brave_ads/browser/ads_service_factory.h"
#include "brave/components/brave_rewards/browser/rewards_service_factory.h"

namespace brave {

void EnsureBrowserContextKeyedServiceFactoriesBuilt() {
  brave_ads::AdsServiceFactory::GetInstance();
  brave_rewards::RewardsServiceFactory::GetInstance();
  TorProfileServiceFactory::GetInstance();
  SearchEngineProviderServiceFactory::GetInstance();
}

}  // namespace brave
