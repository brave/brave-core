// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_ads/buildflags/buildflags.h"
#include "brave/ios/browser/policy/brave_simple_policy_map_ios.h"
#include "components/policy/core/browser/configuration_policy_handler.h"
#include "components/policy/core/browser/configuration_policy_handler_list.h"
#include "ios/chrome/browser/policy/model/configuration_policy_handler_list_factory.h"

#if BUILDFLAG(ENABLE_BRAVE_ADS)
#include "brave/ios/browser/policy/brave_ads_enabled_policy_handler.h"
#endif  // BUILDFLAG(ENABLE_BRAVE_ADS)

namespace brave {

std::unique_ptr<policy::ConfigurationPolicyHandlerList> BuildPolicyHandlerList(
    std::unique_ptr<policy::ConfigurationPolicyHandlerList> handlers) {
  for (const auto& entry : policy::kBraveSimplePolicyMap) {
    handlers->AddHandler(std::make_unique<policy::SimplePolicyHandler>(
        entry.policy_name, entry.preference_path, entry.value_type));
  }
#if BUILDFLAG(ENABLE_BRAVE_ADS)
  handlers->AddHandler(
      std::make_unique<policy::BraveAdsEnabledPolicyHandler>());
#endif  // BUILDFLAG(ENABLE_BRAVE_ADS)
  return handlers;
}

}  // namespace brave
