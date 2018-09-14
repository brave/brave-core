/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/browser_context_keyed_service_factories.h"

#include "brave/components/brave_rewards/browser/rewards_service_factory.h"
#include "brave/browser/tor/tor_profile_service_factory.h"

namespace brave {

void EnsureBrowserContextKeyedServiceFactoriesBuilt() {
  brave_rewards::RewardsServiceFactory::GetInstance();
  TorProfileServiceFactory::GetInstance();
}

}  // namespace brave
