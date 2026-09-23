/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/pref_names.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(BraveWalletPrefsTest, MigratesCryptoWalletsPrefToBraveWallet) {
  TestingPrefServiceSimple prefs;
  prefs.registry()->RegisterIntegerPref(
      kDefaultEthereumWallet,
      static_cast<int>(mojom::DefaultWallet::AskDeprecated));  // default unused

  // Changes CryptoWalletsDeprecated to BraveWalletPreferExtension
  prefs.SetInteger(
      kDefaultEthereumWallet,
      static_cast<int>(mojom::DefaultWallet::CryptoWalletsDeprecated));
  MigrateCryptoWalletsPrefToBraveWallet(&prefs);
  EXPECT_EQ(prefs.GetInteger(kDefaultEthereumWallet),
            static_cast<int>(mojom::DefaultWallet::BraveWalletPreferExtension));

  // These values should not change after migration
  const std::vector<mojom::DefaultWallet> noChangeValues = {
      mojom::DefaultWallet::BraveWallet,
      mojom::DefaultWallet::BraveWalletPreferExtension,
      mojom::DefaultWallet::None};

  for (const auto wallet : noChangeValues) {
    prefs.SetInteger(kDefaultEthereumWallet, static_cast<int>(wallet));
    MigrateCryptoWalletsPrefToBraveWallet(&prefs);
    EXPECT_EQ(prefs.GetInteger(kDefaultEthereumWallet),
              static_cast<int>(wallet))
        << "Wallet type " << static_cast<int>(wallet)
        << " should remain unchanged.";
  }
}

}  // namespace brave_wallet
