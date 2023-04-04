/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_POLICY_BRAVE_SIMPLE_POLICY_MAP_H_
#define BRAVE_BROWSER_POLICY_BRAVE_SIMPLE_POLICY_MAP_H_

#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "build/build_config.h"
#include "components/policy/core/browser/configuration_policy_handler.h"
#include "components/policy/policy_constants.h"

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/brave_wallet/common/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_TOR)
#include "brave/components/tor/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_IPFS)
#include "brave/components/ipfs/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
#include "brave/components/brave_vpn/common/pref_names.h"
#endif

namespace policy {

constexpr PolicyToPreferenceMapEntry kBraveSimplePolicyMap[] = {
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
    {policy::key::kBraveRewardsDisabled,
     brave_rewards::prefs::kDisabledByPolicy, base::Value::Type::BOOLEAN},
    {policy::key::kBraveWalletDisabled, brave_wallet::prefs::kDisabledByPolicy,
     base::Value::Type::BOOLEAN},
    {policy::key::kBraveShieldsDisabledForUrls,
     kManagedBraveShieldsDisabledForUrls, base::Value::Type::LIST},
    {policy::key::kBraveShieldsEnabledForUrls,
     kManagedBraveShieldsEnabledForUrls, base::Value::Type::LIST},
#endif
#if BUILDFLAG(ENABLE_TOR)
    {policy::key::kTorDisabled, tor::prefs::kTorDisabled,
     base::Value::Type::BOOLEAN},
#endif
#if BUILDFLAG(ENABLE_IPFS)
    {policy::key::kIPFSEnabled, kIPFSEnabled, base::Value::Type::BOOLEAN},
#endif
#if BUILDFLAG(ENABLE_BRAVE_VPN)
    {policy::key::kBraveVPNDisabled, brave_vpn::prefs::kManagedBraveVPNDisabled,
     base::Value::Type::BOOLEAN},
#endif
};

}  // namespace policy

#endif  // BRAVE_BROWSER_POLICY_BRAVE_SIMPLE_POLICY_MAP_H_
