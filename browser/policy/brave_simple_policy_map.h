/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_POLICY_BRAVE_SIMPLE_POLICY_MAP_H_
#define BRAVE_BROWSER_POLICY_BRAVE_SIMPLE_POLICY_MAP_H_

#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/brave_wayback_machine/buildflags/buildflags.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/de_amp/common/pref_names.h"
#include "brave/components/debounce/core/common/pref_names.h"
#include "brave/components/global_privacy_control/pref_names.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "brave/components/p3a/pref_names.h"
#include "brave/components/playlist/core/common/pref_names.h"
#include "brave/components/query_filter/pref_names.h"
#include "brave/components/speedreader/common/buildflags/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "build/build_config.h"
#include "components/policy/core/browser/configuration_policy_handler.h"
#include "components/policy/policy_constants.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "brave/components/brave_wallet/common/pref_names.h"

#if BUILDFLAG(ENABLE_TOR)
#include "brave/components/tor/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/common/pref_names.h"
#endif

#if BUILDFLAG(DEPRECATE_IPFS)
#include "brave/components/ipfs/ipfs_prefs.h"  // nogncheck
#endif                                         // BUILDFLAG(DEPRECATE_IPFS)

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "brave/components/speedreader/speedreader_pref_names.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
#include "brave/components/brave_wayback_machine/pref_names.h"
#endif

namespace policy {

inline constexpr PolicyToPreferenceMapEntry kBraveSimplePolicyMap[] = {
    {policy::key::kBraveRewardsDisabled,
     brave_rewards::prefs::kDisabledByPolicy, base::Value::Type::BOOLEAN},
    {policy::key::kBraveWalletDisabled, brave_wallet::prefs::kDisabledByPolicy,
     base::Value::Type::BOOLEAN},
    {policy::key::kBraveShieldsDisabledForUrls,
     kManagedBraveShieldsDisabledForUrls, base::Value::Type::LIST},
    {policy::key::kBraveShieldsEnabledForUrls,
     kManagedBraveShieldsEnabledForUrls, base::Value::Type::LIST},
    {policy::key::kBraveSyncUrl, brave_sync::kCustomSyncServiceUrl,
     base::Value::Type::STRING},

#if BUILDFLAG(ENABLE_TOR)
    {policy::key::kTorDisabled, tor::prefs::kTorDisabled,
     base::Value::Type::BOOLEAN},
#endif
#if BUILDFLAG(ENABLE_BRAVE_VPN)
    {policy::key::kBraveVPNDisabled, brave_vpn::prefs::kManagedBraveVPNDisabled,
     base::Value::Type::BOOLEAN},
#endif
    {policy::key::kBraveAIChatEnabled, ai_chat::prefs::kEnabledByPolicy,
     base::Value::Type::BOOLEAN},
    {policy::key::kBraveP3AEnabled, p3a::kP3AEnabled,
     base::Value::Type::BOOLEAN},
    {policy::key::kBraveStatsPingEnabled, kStatsReportingEnabled,
     base::Value::Type::BOOLEAN},
    {policy::key::kBravePlaylistEnabled, playlist::kPlaylistEnabledPref,
     base::Value::Type::BOOLEAN},
    {policy::key::kBraveWebDiscoveryEnabled, kWebDiscoveryEnabled,
     base::Value::Type::BOOLEAN},
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
    {policy::key::kBraveNewsDisabled,
     brave_news::prefs::kBraveNewsDisabledByPolicy, base::Value::Type::BOOLEAN},
    {policy::key::kBraveTalkDisabled, kBraveTalkDisabledByPolicy,
     base::Value::Type::BOOLEAN},
#if BUILDFLAG(ENABLE_SPEEDREADER)
    {policy::key::kBraveSpeedreaderEnabled, speedreader::kSpeedreaderEnabled,
     base::Value::Type::BOOLEAN},
#endif
#endif
#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
    {policy::key::kBraveWaybackMachineEnabled, kBraveWaybackMachineEnabled,
     base::Value::Type::BOOLEAN},
#endif
#if BUILDFLAG(DEPRECATE_IPFS)
    {policy::key::kIPFSEnabled, ipfs::prefs::kIPFSEnabledByPolicy,
     base::Value::Type::BOOLEAN},
#endif  // BUILDFLAG(DEPRECATE_IPFS)
    {policy::key::kBraveReduceLanguageEnabled,
     brave_shields::prefs::kReduceLanguageEnabled, base::Value::Type::BOOLEAN},
    {policy::key::kBraveDeAmpEnabled, de_amp::kDeAmpPrefEnabled,
     base::Value::Type::BOOLEAN},
    {policy::key::kBraveDebouncingEnabled, debounce::prefs::kDebounceEnabled,
     base::Value::Type::BOOLEAN},
    {policy::key::kDefaultBraveFingerprintingV2Setting,
     kManagedDefaultBraveFingerprintingV2, base::Value::Type::INTEGER},
    {policy::key::kBraveTrackingQueryParametersFilteringEnabled,
     query_filter::kTrackingQueryParametersFilteringEnabled,
     base::Value::Type::BOOLEAN},
    {policy::key::kBraveGlobalPrivacyControlEnabled,
     global_privacy_control::kGlobalPrivacyControlEnabled,
     base::Value::Type::BOOLEAN},
};

}  // namespace policy

#endif  // BRAVE_BROWSER_POLICY_BRAVE_SIMPLE_POLICY_MAP_H_
