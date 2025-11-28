/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_FEATURES_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_FEATURES_H_

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "brave/components/brave_wallet/common/buildflags/buildflags.h"

static_assert(BUILDFLAG(ENABLE_BRAVE_WALLET));
namespace brave_wallet::features {

BASE_DECLARE_FEATURE(kNativeBraveWalletFeature);

BASE_DECLARE_FEATURE(kBraveWalletBitcoinFeature);
extern const base::FeatureParam<int> kBitcoinRpcThrottle;
extern const base::FeatureParam<bool> kBitcoinTestnetDiscovery;
BASE_DECLARE_FEATURE(kBraveWalletBitcoinImportFeature);
BASE_DECLARE_FEATURE(kBraveWalletBitcoinLedgerFeature);

BASE_DECLARE_FEATURE(kBraveWalletZCashFeature);
// Adds shielded operations support for Z Cash
extern const base::FeatureParam<bool> kZCashShieldedTransactionsEnabled;

BASE_DECLARE_FEATURE(kBraveWalletPolkadotFeature);

BASE_DECLARE_FEATURE(kBraveWalletCardanoFeature);
extern const base::FeatureParam<int> kCardanoRpcThrottle;
extern const base::FeatureParam<bool> kCardanoDAppSupport;

BASE_DECLARE_FEATURE(kBraveWalletAnkrBalancesFeature);
BASE_DECLARE_FEATURE(kBraveWalletTransactionSimulationsFeature);

}  // namespace brave_wallet::features

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_FEATURES_H_
