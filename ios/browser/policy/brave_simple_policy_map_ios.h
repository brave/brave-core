/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_POLICY_BRAVE_SIMPLE_POLICY_MAP_IOS_H_
#define BRAVE_IOS_BROWSER_POLICY_BRAVE_SIMPLE_POLICY_MAP_IOS_H_

#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/p3a/pref_names.h"
#include "brave/components/playlist/core/common/pref_names.h"
#include "build/build_config.h"
#include "components/policy/core/browser/configuration_policy_handler.h"
#include "components/policy/policy_constants.h"

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/common/pref_names.h"
#endif

namespace policy {

inline constexpr PolicyToPreferenceMapEntry kBraveSimplePolicyMap[] = {
    {
        policy::key::kBraveWalletDisabled,
        brave_wallet::kBraveWalletDisabledByPolicy,
        base::Value::Type::BOOLEAN,
    },
    {
        policy::key::kBraveAIChatEnabled,
        ai_chat::prefs::kEnabledByPolicy,
        base::Value::Type::BOOLEAN,
    },
    {
        policy::key::kBraveRewardsDisabled,
        brave_rewards::prefs::kDisabledByPolicy,
        base::Value::Type::BOOLEAN,
    },
    {
        policy::key::kBraveTalkDisabled,
        kBraveTalkDisabledByPolicy,
        base::Value::Type::BOOLEAN,
    },
    {
        policy::key::kBraveNewsDisabled,
        brave_news::prefs::kBraveNewsDisabledByPolicy,
        base::Value::Type::BOOLEAN,
    },
#if BUILDFLAG(ENABLE_BRAVE_VPN)
    {
        policy::key::kBraveVPNDisabled,
        brave_vpn::prefs::kManagedBraveVPNDisabled,
        base::Value::Type::BOOLEAN,
    },
#endif
    {
        policy::key::kBraveP3AEnabled,
        p3a::kP3AEnabled,
        base::Value::Type::BOOLEAN,
    },
    {
        policy::key::kBraveStatsPingEnabled,
        kStatsReportingEnabled,
        base::Value::Type::BOOLEAN,
    },
    {
        policy::key::kBravePlaylistEnabled,
        playlist::kPlaylistEnabledPref,
        base::Value::Type::BOOLEAN,
    },
};

}  // namespace policy

#endif  // BRAVE_IOS_BROWSER_POLICY_BRAVE_SIMPLE_POLICY_MAP_IOS_H_
