/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/feature_override.h"
#include "brave/components/brave_rewards/common/features.h"
#include "brave/components/brave_search_conversion/features.h"
#include "brave/components/brave_today/common/features.h"
#include "brave/components/brave_wallet/common/features.h"
#include "third_party/blink/public/common/features.h"

// clang-format off
#define kForceWebContentsDarkMode kForceWebContentsDarkMode, \
    &brave_rewards::features::kBraveRewards,                 \
    &brave_search_conversion::features::kOmniboxBanner,      \
    &brave_today::features::kBraveNewsFeature,               \
    &brave_wallet::features::kNativeBraveWalletFeature,      \
    &brave_wallet::features::kBraveWalletSolanaFeature
// clang-format on

#include "src/chrome/browser/flags/android/chrome_feature_list.cc"
#undef kForceWebContentsDarkMode

namespace chrome {
namespace android {

OVERRIDE_FEATURE_DEFAULT_STATES({{
    {kAddToHomescreenIPH, base::FEATURE_DISABLED_BY_DEFAULT},
    {kStartSurfaceAndroid, base::FEATURE_DISABLED_BY_DEFAULT},
    {kTabSwitcherOnReturn, base::FEATURE_DISABLED_BY_DEFAULT},
}});

}  // namespace android
}  // namespace chrome
