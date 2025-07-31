/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_origin/brave_origin_profile_prefs.h"

#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_origin/brave_origin_state.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/brave_wallet/common/pref_names.h"
#include "brave/components/brave_wayback_machine/buildflags/buildflags.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/p3a/pref_names.h"
#include "brave/components/sidebar/browser/pref_names.h"
#include "brave/components/sidebar/browser/sidebar_item.h"
#include "brave/components/speedreader/common/buildflags/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "components/metrics/metrics_pref_names.h"
#include "components/prefs/pref_service.h"

#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
#include "brave/components/brave_wayback_machine/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "brave/components/speedreader/speedreader_pref_names.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/common/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_TOR)
#include "brave/components/tor/pref_names.h"
#endif

namespace brave_origin {

void SetupBraveOriginProfilePrefs(Profile* profile) {
  if (!profile || profile->IsIncognitoProfile()) {
    return;
  }

  if (BraveOriginState::GetInstance()->IsBraveOriginUser()) {
    PrefService* prefs = profile->GetPrefs();

    // TODO(https://github.com/brave/brave-browser/issues/48145)
    // Need to find out which features are actively being used.
    // Those should be enabled / visible.

    // Profile prefs
    prefs->SetDefaultPrefValue(brave_rewards::prefs::kDisabledByPolicy,
                               base::Value(true));
    prefs->SetDefaultPrefValue(ai_chat::prefs::kEnabledByPolicy,
                               base::Value(false));
    prefs->SetDefaultPrefValue(brave_news::prefs::kBraveNewsDisabledByPolicy,
                               base::Value(true));
#if BUILDFLAG(ENABLE_BRAVE_VPN)
    prefs->SetDefaultPrefValue(brave_vpn::prefs::kManagedBraveVPNDisabled,
                               base::Value(true));
#endif
    prefs->SetDefaultPrefValue(brave_wallet::prefs::kDisabledByPolicy,
                               base::Value(true));
    prefs->SetDefaultPrefValue(kBraveTalkDisabledByPolicy, base::Value(true));
#if BUILDFLAG(ENABLE_SPEEDREADER)
    prefs->SetDefaultPrefValue(speedreader::kSpeedreaderDisabledByPolicy,
                               base::Value(true));
#endif
#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
    prefs->SetDefaultPrefValue(kBraveWaybackMachineDisabledByPolicy,
                               base::Value(true));
#endif
    prefs->SetDefaultPrefValue(kWebDiscoveryDisabledByPolicy,
                               base::Value(true));

    // Local state (spans all profiles)
    g_browser_process->local_state()->SetDefaultPrefValue(
        p3a::kP3ADisabledByPolicy, base::Value(true));
    g_browser_process->local_state()->SetDefaultPrefValue(
        kStatsReportingDisabledByPolicy, base::Value(true));
    g_browser_process->local_state()->SetDefaultPrefValue(
        metrics::prefs::kMetricsReportingEnabled, base::Value(false));
#if BUILDFLAG(ENABLE_TOR)
    g_browser_process->local_state()->SetDefaultPrefValue(
        tor::prefs::kTorDisabled, base::Value(true));
#endif
  }
}

}  // namespace brave_origin
