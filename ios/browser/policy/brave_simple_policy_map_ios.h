/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_POLICY_BRAVE_SIMPLE_POLICY_MAP_H_
#define BRAVE_IOS_BROWSER_POLICY_BRAVE_SIMPLE_POLICY_MAP_H_

#include "brave/components/constants/pref_names.h"
#include "build/build_config.h"
#include "components/policy/core/browser/configuration_policy_handler.h"
#include "components/policy/policy_constants.h"
#include "brave/components/brave_wallet/common/pref_names.h"

namespace policy {

inline constexpr PolicyToPreferenceMapEntry kBraveSimplePolicyMap[] = {
    {policy::key::kBraveWalletDisabled, brave_wallet::prefs::kDisabledByPolicy,
     base::Value::Type::BOOLEAN},
};

}  // namespace policy

#endif  // BRAVE_IOS_BROWSER_POLICY_BRAVE_SIMPLE_POLICY_MAP_H_
