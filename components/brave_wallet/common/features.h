/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_FEATURES_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_FEATURES_H_

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace brave_wallet {
namespace features {

BASE_DECLARE_FEATURE(kNativeBraveWalletFeature);
BASE_DECLARE_FEATURE(kBraveWalletFilecoinFeature);
BASE_DECLARE_FEATURE(kBraveWalletSolanaFeature);
extern const base::FeatureParam<bool> kCreateDefaultSolanaAccount;
BASE_DECLARE_FEATURE(kBraveWalletSolanaProviderFeature);
BASE_DECLARE_FEATURE(kBraveWalletDappsSupportFeature);
BASE_DECLARE_FEATURE(kBraveWalletENSL2Feature);

}  // namespace features
}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_FEATURES_H_
