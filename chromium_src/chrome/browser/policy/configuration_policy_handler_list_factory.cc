/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/policy/configuration_policy_handler_list_factory.h"

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

namespace {

const policy::PolicyToPreferenceMapEntry kBraveSimplePolicyMap[] = {
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
    {policy::key::kBraveRewardsDisabled,
     brave_rewards::prefs::kDisabledByPolicy, base::Value::Type::BOOLEAN},
    {policy::key::kBraveWalletDisabled, brave_wallet::prefs::kDisabledByPolicy,
     base::Value::Type::BOOLEAN},
    {policy::key::kShieldsBlockedForUrls, kManagedShieldsBlockedForUrls,
     base::Value::Type::LIST},
    {policy::key::kShieldsAllowedForUrls, kManagedShieldsAllowedForUrls,
     base::Value::Type::LIST},
#endif
#if BUILDFLAG(ENABLE_TOR)
    {policy::key::kTorDisabled, tor::prefs::kTorDisabled,
     base::Value::Type::BOOLEAN},
#endif
#if BUILDFLAG(ENABLE_IPFS)
    {policy::key::kIPFSEnabled, kIPFSEnabled, base::Value::Type::BOOLEAN},
#endif
};

}  // namespace

#define BuildHandlerList BuildHandlerList_ChromiumImpl
#include "src/chrome/browser/policy/configuration_policy_handler_list_factory.cc"
#undef BuildHandlerList

namespace policy {

std::unique_ptr<ConfigurationPolicyHandlerList> BuildHandlerList(
    const Schema& chrome_schema) {
  std::unique_ptr<ConfigurationPolicyHandlerList> handlers =
      BuildHandlerList_ChromiumImpl(chrome_schema);

  for (const auto& entry : kBraveSimplePolicyMap) {
    handlers->AddHandler(std::make_unique<SimplePolicyHandler>(
        entry.policy_name, entry.preference_path, entry.value_type));
  }

  return handlers;
}

}  // namespace policy
