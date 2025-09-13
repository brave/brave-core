/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_origin/brave_origin_service_factory.h"

#include <string_view>

#include "base/containers/fixed_flat_map.h"
#include "base/containers/map_util.h"
#include "base/no_destructor.h"
#include "brave/browser/policy/brave_simple_policy_map.h"
#include "brave/components/brave_origin/brave_origin_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_selections.h"

namespace brave_origin {

// static
BraveOriginService* BraveOriginServiceFactory::GetForProfile(Profile* profile) {
  return static_cast<BraveOriginService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
BraveOriginServiceFactory* BraveOriginServiceFactory::GetInstance() {
  static base::NoDestructor<BraveOriginServiceFactory> instance;
  return instance.get();
}

BraveOriginServiceFactory::BraveOriginServiceFactory()
    : ProfileKeyedServiceFactory(
          "BraveOriginService",
          ProfileSelections::BuildRedirectedInIncognito()) {}

BraveOriginServiceFactory::~BraveOriginServiceFactory() = default;

std::unique_ptr<KeyedService>
BraveOriginServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  // Pass the profile ID here
  return std::make_unique<BraveOriginService>("");
}

bool BraveOriginServiceFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}

// static
BraveOriginPolicyMap
BraveOriginServiceFactory::BuildBraveOriginPolicyDefinitions() {
  BraveOriginPolicyMap policy_definitions;

  // Get the policy map from the policy system
  const auto& policy_map = policy::kBraveSimplePolicyMap;

  // Define BraveOrigin-specific metadata for each pref we want to control
  static constexpr auto brave_origin_metadata =
      base::MakeFixedFlatMap<std::string_view, BraveOriginPrefMetadata>({

          // Wayback Machine preferences
          {kBraveWaybackMachineEnabled,
           BraveOriginPrefMetadata(false, BraveOriginPolicyScope::kProfile,
                                   /*user_settable=*/true)},

#if BUILDFLAG(ENABLE_TOR)
          // Tor preferences
          {tor::prefs::kTorDisabled,
           BraveOriginPrefMetadata(true, BraveOriginPolicyScope::kGlobal,
                                   /*user_settable=*/false)},
#endif

          // Stats reporting
          {kStatsReportingEnabled,
           BraveOriginPrefMetadata(false, BraveOriginPolicyScope::kGlobal,
                                   /*user_settable=*/true)},

          // P3A preferences
          {p3a::kP3AEnabled,
           BraveOriginPrefMetadata(false, BraveOriginPolicyScope::kGlobal,
                                   /*user_settable=*/true)},

          // Brave Rewards preferences
          {brave_rewards::prefs::kDisabledByPolicy,
           BraveOriginPrefMetadata(true, BraveOriginPolicyScope::kProfile,
                                   /*user_settable=*/false)},

          // Brave Wallet preferences
          {brave_wallet::prefs::kDisabledByPolicy,
           BraveOriginPrefMetadata(true, BraveOriginPolicyScope::kProfile,
                                   /*user_settable=*/false)},

          // AI Chat preferences
          {ai_chat::prefs::kEnabledByPolicy,
           BraveOriginPrefMetadata(false, BraveOriginPolicyScope::kProfile,
                                   /*user_settable=*/false)},

#if BUILDFLAG(ENABLE_SPEEDREADER)
          // Speedreader preferences
          {speedreader::kSpeedreaderPrefFeatureEnabled,
           BraveOriginPrefMetadata(false, BraveOriginPolicyScope::kProfile,
                                   /*user_settable=*/true)},
#endif

          // Brave News preferences
          {brave_news::prefs::kBraveNewsDisabledByPolicy,
           BraveOriginPrefMetadata(true, BraveOriginPolicyScope::kProfile,
                                   /*user_settable=*/false)},

#if BUILDFLAG(ENABLE_BRAVE_VPN)
          // Brave VPN preferences
          {brave_vpn::prefs::kManagedBraveVPNDisabled,
           BraveOriginPrefMetadata(true, BraveOriginPolicyScope::kProfile,
                                   /*user_settable=*/false)},
#endif

          // Brave Talk preferences
          {kBraveTalkDisabledByPolicy,
           BraveOriginPrefMetadata(true, BraveOriginPolicyScope::kProfile,
                                   /*user_settable=*/false)},
      });

  // Build the final preference definitions
  for (const auto& [policy_key, pref_name, type] : policy_map) {
    if (const auto* metadata =
            base::FindOrNull(brave_origin_metadata, pref_name)) {
      // brave_origin_pref_key will be computed dynamically at usage time
      // For global prefs: use pref_name directly
      // For profile prefs: use profile_id.pref_name (computed by
      // BraveOriginService)
      std::string brave_origin_pref_key = pref_name;

      policy_definitions.emplace(
          pref_name,
          BraveOriginPolicyInfo(pref_name, metadata->origin_default_value,
                                metadata->scope, metadata->user_settable,
                                policy_key, brave_origin_pref_key));
    }
  }

  return policy_definitions;
}

}  // namespace brave_origin
