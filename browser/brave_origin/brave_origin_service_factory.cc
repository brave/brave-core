/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_origin/brave_origin_service_factory.h"

#include <memory>

#include "base/base64url.h"
#include "base/no_destructor.h"
#include "brave/browser/policy/brave_simple_policy_map.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_origin/brave_origin_prefs.h"
#include "brave/components/brave_origin/brave_origin_service.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/brave_wallet/common/pref_names.h"
#include "brave/components/brave_wayback_machine/buildflags/buildflags.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/p3a/pref_names.h"
#include "brave/components/speedreader/common/buildflags/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/policy/profile_policy_connector.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_selections.h"
#include "components/policy/core/common/policy_service.h"

#if BUILDFLAG(ENABLE_TOR)
#include "brave/components/tor/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/common/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "brave/components/speedreader/speedreader_pref_names.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
#include "brave/components/brave_wayback_machine/pref_names.h"
#endif

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
          ProfileSelections::BuildForRegularAndIncognito()) {}

BraveOriginServiceFactory::~BraveOriginServiceFactory() = default;

std::unique_ptr<KeyedService>
BraveOriginServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);

  // Create a base64url encoded profile identifier for stable, safe identifier
  std::string path_basename = profile->GetBaseName().AsUTF8Unsafe();
  std::string profile_id;
  base::Base64UrlEncode(
      path_basename, base::Base64UrlEncodePolicy::INCLUDE_PADDING, &profile_id);

  // Build the pref definitions with all necessary dependencies
  auto pref_definitions = BuildBraveOriginPrefDefinitions();

  // Get policy service from profile
  policy::PolicyService* policy_service = nullptr;
  if (auto* connector = profile->GetProfilePolicyConnector()) {
    policy_service = connector->policy_service();
  }

  auto brave_origin_service = std::make_unique<BraveOriginService>(
      g_browser_process->local_state(), profile->GetPrefs(), profile_id,
      std::move(pref_definitions), policy_service);

  return brave_origin_service;
}

bool BraveOriginServiceFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}

// static
BraveOriginPrefMap
BraveOriginServiceFactory::BuildBraveOriginPrefDefinitions() {
  BraveOriginPrefMap pref_definitions;

  // Get the policy map from the policy system
  const auto& policy_map = policy::kBraveSimplePolicyMap;

  // Define BraveOrigin-specific metadata for each pref we want to control
  base::flat_map<std::string, BraveOriginPrefMetadata> brave_origin_metadata;

  // Wayback Machine preferences
  brave_origin_metadata.emplace(
      kBraveWaybackMachineEnabled,
      BraveOriginPrefMetadata(base::Value(false),
                              BraveOriginPolicyScope::kProfile,
                              /*user_settable=*/true));

#if BUILDFLAG(ENABLE_TOR)
  // Tor preferences
  brave_origin_metadata.emplace(
      tor::prefs::kTorDisabled,
      BraveOriginPrefMetadata(base::Value(true),
                              BraveOriginPolicyScope::kGlobal,
                              /*user_settable=*/false));
#endif

  // Stats reporting
  brave_origin_metadata.emplace(
      kStatsReportingEnabled,
      BraveOriginPrefMetadata(base::Value(false),
                              BraveOriginPolicyScope::kGlobal,
                              /*user_settable=*/true));

  // P3A preferences
  brave_origin_metadata.emplace(
      p3a::kP3AEnabled, BraveOriginPrefMetadata(base::Value(false),
                                                BraveOriginPolicyScope::kGlobal,
                                                /*user_settable=*/true));

  // Brave Rewards preferences
  brave_origin_metadata.emplace(
      brave_rewards::prefs::kDisabledByPolicy,
      BraveOriginPrefMetadata(base::Value(true),
                              BraveOriginPolicyScope::kProfile,
                              /*user_settable=*/true));

  // Brave Wallet preferences
  brave_origin_metadata.emplace(
      brave_wallet::prefs::kDisabledByPolicy,
      BraveOriginPrefMetadata(base::Value(true),
                              BraveOriginPolicyScope::kProfile,
                              /*user_settable=*/false));

  // AI Chat preferences
  brave_origin_metadata.emplace(
      ai_chat::prefs::kEnabledByPolicy,
      BraveOriginPrefMetadata(base::Value(false),
                              BraveOriginPolicyScope::kProfile,
                              /*user_settable=*/false));

#if BUILDFLAG(ENABLE_SPEEDREADER)
  // Speedreader preferences
  brave_origin_metadata.emplace(
      speedreader::kSpeedreaderPrefFeatureEnabled,
      BraveOriginPrefMetadata(base::Value(false),
                              BraveOriginPolicyScope::kProfile,
                              /*user_settable=*/true));
#endif

  // Brave News preferences
  brave_origin_metadata.emplace(
      brave_news::prefs::kBraveNewsDisabledByPolicy,
      BraveOriginPrefMetadata(base::Value(true),
                              BraveOriginPolicyScope::kProfile,
                              /*user_settable=*/false));

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  // Brave VPN preferences
  brave_origin_metadata.emplace(
      brave_vpn::prefs::kManagedBraveVPNDisabled,
      BraveOriginPrefMetadata(base::Value(true),
                              BraveOriginPolicyScope::kProfile,
                              /*user_settable=*/true));
#endif

  // Brave Talk preferences
  brave_origin_metadata.emplace(
      kBraveTalkDisabledByPolicy,
      BraveOriginPrefMetadata(base::Value(true),
                              BraveOriginPolicyScope::kProfile,
                              /*user_settable=*/false));

  // Build the final preference definitions
  for (const auto& [policy_key, pref_name, type] : policy_map) {
    auto metadata_it = brave_origin_metadata.find(pref_name);
    if (metadata_it != brave_origin_metadata.end()) {
      const auto& metadata = metadata_it->second;
      pref_definitions.emplace(
          pref_name, BraveOriginPrefInfo(
                         pref_name, metadata.origin_default_value.Clone(),
                         metadata.scope, metadata.user_settable, policy_key));
    }
  }

  return pref_definitions;
}

// static
base::flat_map<std::string, std::string>
BraveOriginServiceFactory::GetPolicyKeyMappings() {
  base::flat_map<std::string, std::string> mappings;

  auto pref_definitions = BuildBraveOriginPrefDefinitions();

  for (const auto& [pref_name, pref_info] : pref_definitions) {
    mappings[pref_info.policy_key] = pref_name;
  }

  return mappings;
}

// static
void BraveOriginServiceFactory::InitializeBraveOriginPrefs() {
  BraveOriginPrefs::GetInstance()->Init(BuildBraveOriginPrefDefinitions(),
                                        GetPolicyKeyMappings());
}

}  // namespace brave_origin
