/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_substrate_keyring.h"

#include <memory>
#include <utility>

#include "brave/components/brave_wallet/browser/bip39.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {
constexpr char kBip84TestMnemonic[] =
    "immense leader act drama someone renew become mention fragile wide "
    "cinnamon obtain wool window mirror";
}  // namespace

TEST(PolkadotSubstrateKeyringTest, AddNewHDAccount) {
  PolkadotSubstrateKeyring keyring(*bip39::MnemonicToSeed(kBip84TestMnemonic));

  EXPECT_EQ(keyring.AddNewHDAccount(0).value(),
            "5FxTF5Tbe8xSzh8jRwBdy2o11tZrdyv7PQM7HFXBarrYDiDa");

  EXPECT_EQ(keyring.AddNewHDAccount(1), std::nullopt);
}

}  // namespace brave_wallet
