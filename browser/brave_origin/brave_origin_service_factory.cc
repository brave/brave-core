/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_origin/brave_origin_service_factory.h"

#include <string_view>

#include "base/check_is_test.h"
#include "base/containers/fixed_flat_map.h"
#include "base/containers/map_util.h"
#include "base/no_destructor.h"
#include "brave/browser/policy/brave_simple_policy_map.h"
#include "brave/components/brave_origin/brave_origin_policy_manager.h"
#include "brave/components/brave_origin/brave_origin_service.h"
#include "brave/components/brave_origin/profile_id.h"
#include "brave/components/brave_wayback_machine/pref_names.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/p3a/pref_names.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_selections.h"
#include "components/policy/core/browser/browser_policy_connector_base.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/brave_wallet/common/pref_names.h"

#if BUILDFLAG(ENABLE_TOR)
#include "brave/components/tor/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "brave/components/speedreader/speedreader_pref_names.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/common/pref_names.h"
#endif

namespace policy {
class PolicyService;
}

namespace policy {
class ChromeBrowserPolicyConnector;
}

namespace brave_origin {

// Defined in chromium_src/chrome/browser/profiles/profile.cc to avoid a
// circular dep on chrome/browser for the policy connector include.
policy::PolicyService* GetPolicyServiceFromProfile(Profile* profile);

policy::PolicyService* GetBrowserPolicyService() {
  if (!g_browser_process || !g_browser_process->browser_policy_connector()) {
    CHECK_IS_TEST();
    return nullptr;
  }

  auto* connector = reinterpret_cast<policy::BrowserPolicyConnectorBase*>(
      g_browser_process->browser_policy_connector());
  // Only get policy service if we have one already or not in testing mode
  if (connector->HasPolicyService()) {
    return connector->GetPolicyService();
  }
  return nullptr;
}

namespace {

// Define BraveOrigin-specific metadata for browser-level prefs
constexpr auto kBraveOriginBrowserMetadata =
    base::MakeFixedFlatMap<std::string_view,
                           BraveOriginServiceFactory::BraveOriginPrefMetadata>({

#if BUILDFLAG(ENABLE_TOR)
        // Tor preferences
        {tor::prefs::kTorDisabled,
         BraveOriginServiceFactory::BraveOriginPrefMetadata(
             true,
             /*user_settable=*/false)},
#endif

        // Stats reporting
        {kStatsReportingEnabled,
         BraveOriginServiceFactory::BraveOriginPrefMetadata(
             false,
             /*user_settable=*/true)},

        // P3A preferences
        {p3a::kP3AEnabled, BraveOriginServiceFactory::BraveOriginPrefMetadata(
                               false,
                               /*user_settable=*/true)},
    });

// Define BraveOrigin-specific metadata for profile-level prefs
constexpr auto kBraveOriginProfileMetadata =
    base::MakeFixedFlatMap<std::string_view,
                           BraveOriginServiceFactory::BraveOriginPrefMetadata>({
        // Wayback Machine preferences
        {kBraveWaybackMachineEnabled,
         BraveOriginServiceFactory::BraveOriginPrefMetadata(
             false,
             /*user_settable=*/true)},

        // Brave Rewards preferences
        {brave_rewards::prefs::kDisabledByPolicy,
         BraveOriginServiceFactory::BraveOriginPrefMetadata(
             true,
             /*user_settable=*/false)},

        // Brave Wallet preferences
        {brave_wallet::prefs::kDisabledByPolicy,
         BraveOriginServiceFactory::BraveOriginPrefMetadata(
             true,
             /*user_settable=*/false)},
        // AI Chat preferences
        {ai_chat::prefs::kEnabledByPolicy,
         BraveOriginServiceFactory::BraveOriginPrefMetadata(
             false,
             /*user_settable=*/false)},

#if BUILDFLAG(ENABLE_SPEEDREADER)
        // Speedreader preferences
        {speedreader::kSpeedreaderEnabled,
         BraveOriginServiceFactory::BraveOriginPrefMetadata(
             false,
             /*user_settable=*/true)},
#endif

        // Brave News preferences
        {brave_news::prefs::kBraveNewsDisabledByPolicy,
         BraveOriginServiceFactory::BraveOriginPrefMetadata(
             true,
             /*user_settable=*/false)},

#if BUILDFLAG(ENABLE_BRAVE_VPN)
        // Brave VPN preferences
        {brave_vpn::prefs::kManagedBraveVPNDisabled,
         BraveOriginServiceFactory::BraveOriginPrefMetadata(
             true,
             /*user_settable=*/false)},
#endif

        // Brave Talk preferences
        {kBraveTalkDisabledByPolicy,
         BraveOriginServiceFactory::BraveOriginPrefMetadata(
             true,
             /*user_settable=*/false)},

        // Web Discovery preferences
        {kWebDiscoveryEnabled,
         BraveOriginServiceFactory::BraveOriginPrefMetadata(
             false,
             /*user_settable=*/true)},
    });

}  // namespace

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
  Profile* profile = Profile::FromBrowserContext(context);

  // Lazy initialization of BraveOriginPolicyManager
  auto* policy_manager = BraveOriginPolicyManager::GetInstance();
  if (!policy_manager->IsInitialized()) {
    policy_manager->Init(GetBrowserPolicyDefinitions(),
                         GetProfilePolicyDefinitions(),
                         g_browser_process->local_state());
  }

  std::string profile_id = GetProfileId(profile->GetPath());
  return std::make_unique<BraveOriginService>(
      g_browser_process->local_state(), profile->GetPrefs(), profile_id,
      GetPolicyServiceFromProfile(profile), GetBrowserPolicyService());
}

bool BraveOriginServiceFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}

// static
BraveOriginPolicyMap BraveOriginServiceFactory::GetBrowserPolicyDefinitions() {
  BraveOriginPolicyMap browser_policy_definitions;

  // Build browser-level preference definitions
  for (const auto& [policy_key, pref_name, type] :
       policy::kBraveSimplePolicyMap) {
    if (const auto* metadata =
            base::FindOrNull(kBraveOriginBrowserMetadata, pref_name)) {
      std::string brave_origin_pref_key = pref_name;

      browser_policy_definitions.emplace(
          policy_key, BraveOriginPolicyInfo(
                          pref_name, metadata->origin_default_value,
                          metadata->user_settable, brave_origin_pref_key));
    }
  }

  return browser_policy_definitions;
}

// static
BraveOriginPolicyMap BraveOriginServiceFactory::GetProfilePolicyDefinitions() {
  BraveOriginPolicyMap profile_policy_definitions;

  // Build profile-level preference definitions
  for (const auto& [policy_key, pref_name, type] :
       policy::kBraveSimplePolicyMap) {
    if (const auto* metadata =
            base::FindOrNull(kBraveOriginProfileMetadata, pref_name)) {
      std::string brave_origin_pref_key = pref_name;

      profile_policy_definitions.emplace(
          policy_key, BraveOriginPolicyInfo(
                          pref_name, metadata->origin_default_value,
                          metadata->user_settable, brave_origin_pref_key));
    }
  }

  return profile_policy_definitions;
}

}  // namespace brave_origin
