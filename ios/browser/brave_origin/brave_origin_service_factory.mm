/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/brave_origin/brave_origin_service_factory.h"

#include <memory>

#include "base/containers/fixed_flat_map.h"
#include "base/containers/map_util.h"
#include "base/no_destructor.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_origin/brave_origin_policy_manager.h"
#include "brave/components/brave_origin/brave_origin_service.h"
#include "brave/components/brave_origin/profile_id.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/p3a/pref_names.h"
#include "brave/ios/browser/policy/brave_simple_policy_map_ios.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/user_prefs/user_prefs.h"
#include "ios/chrome/browser/policy/model/browser_policy_connector_ios.h"
#include "ios/chrome/browser/policy/model/profile_policy_connector.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"

#if BUILDFLAG(ENABLE_AI_CHAT)
#include "brave/components/ai_chat/core/common/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
#include "brave/components/brave_wallet/browser/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_TALK)
#include "brave/components/brave_talk/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/common/pref_names.h"
#endif

namespace brave_origin {

namespace {

// Define BraveOrigin-specific metadata for browser-level prefs
constexpr auto kBraveOriginBrowserMetadata =
    base::MakeFixedFlatMap<std::string_view,
                           BraveOriginServiceFactory::BraveOriginPrefMetadata>({
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
        // Brave Rewards preferences
        {brave_rewards::prefs::kDisabledByPolicy,
         BraveOriginServiceFactory::BraveOriginPrefMetadata(
             true,
             /*user_settable=*/false)},

#if BUILDFLAG(ENABLE_BRAVE_WALLET)
        // Brave Wallet preferences
        {brave_wallet::kBraveWalletDisabledByPolicy,
         BraveOriginServiceFactory::BraveOriginPrefMetadata(
             true,
             /*user_settable=*/false)},
#endif

#if BUILDFLAG(ENABLE_AI_CHAT)
        // AI Chat preferences
        {ai_chat::prefs::kEnabledByPolicy,
         BraveOriginServiceFactory::BraveOriginPrefMetadata(
             false,
             /*user_settable=*/false)},
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

#if BUILDFLAG(ENABLE_BRAVE_TALK)
        // Brave Talk preferences
        {brave_talk::prefs::kDisabledByPolicy,
         BraveOriginServiceFactory::BraveOriginPrefMetadata(
             true,
             /*user_settable=*/false)},
#endif
    });

}  // namespace

// static
BraveOriginService* BraveOriginServiceFactory::GetForProfile(
    ProfileIOS* profile) {
  return GetInstance()->GetServiceForProfileAs<BraveOriginService>(profile,
                                                                   true);
}

// static
BraveOriginServiceFactory* BraveOriginServiceFactory::GetInstance() {
  static base::NoDestructor<BraveOriginServiceFactory> instance;
  return instance.get();
}

BraveOriginServiceFactory::BraveOriginServiceFactory()
    : ProfileKeyedServiceFactoryIOS("BraveOriginService",
                                    ProfileSelection::kRedirectedInIncognito,
                                    ServiceCreation::kCreateWithProfile,
                                    TestingCreation::kNoServiceForTests) {}

BraveOriginServiceFactory::~BraveOriginServiceFactory() = default;

std::unique_ptr<KeyedService>
BraveOriginServiceFactory::BuildServiceInstanceFor(ProfileIOS* profile) const {
  // Lazy initialization of BraveOriginPolicyManager
  auto* policy_manager = BraveOriginPolicyManager::GetInstance();
  if (!policy_manager->IsInitialized()) {
    policy_manager->Init(GetBrowserPolicyDefinitions(),
                         GetProfilePolicyDefinitions(),
                         GetApplicationContext()->GetLocalState());
  }

  std::string profile_id = GetProfileId(profile->GetStatePath());
  return std::make_unique<BraveOriginService>(
      GetApplicationContext()->GetLocalState(),
      user_prefs::UserPrefs::Get(profile), profile_id,
      profile->GetPolicyConnector()->GetPolicyService(),
      GetApplicationContext()->GetBrowserPolicyConnector()->GetPolicyService());
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
