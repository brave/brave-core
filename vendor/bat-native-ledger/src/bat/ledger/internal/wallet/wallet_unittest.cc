/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/wallet/wallet.h"

#include <vector>

#include "bat/ledger/internal/core/bat_ledger_test.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/state/state_keys.h"

namespace ledger {

class WalletTest : public BATLedgerTest {};

TEST_F(WalletTest, GetWallet) {
  auto* ledger = GetLedgerImpl();

  // A wallet is created if none exists
  ledger->ledger_client()->SetStringState(state::kWalletBrave, "");
  auto wallet = ledger->wallet()->GetWallet(true);
  ASSERT_TRUE(wallet);
  EXPECT_TRUE(wallet->payment_id.empty());
  EXPECT_TRUE(!wallet->recovery_seed.empty());

  // The created wallet is saved
  std::vector<uint8_t> recovery_seed = wallet->recovery_seed;
  wallet = ledger->wallet()->GetWallet(true);
  ASSERT_TRUE(wallet);
  EXPECT_EQ(wallet->recovery_seed, recovery_seed);

  // Corrupted wallet data is not overwritten
  ledger->ledger_client()->SetStringState(state::kWalletBrave, "BAD-DATA");
  wallet = ledger->wallet()->GetWallet(true);
  ASSERT_FALSE(wallet);
}

TEST_F(WalletTest, CreateWallet) {
  auto* ledger = GetLedgerImpl();

  ledger->ledger_client()->SetStringState(state::kWalletBrave, "BAD-DATA");

  mojom::Result result;
  ledger->wallet()->CreateWalletIfNecessary(
      [&result](mojom::Result r) mutable { result = r; });

  // Corrupted wallet data is not overwritten with a new wallet
  EXPECT_EQ(result, mojom::Result::LEDGER_ERROR);
  EXPECT_EQ(ledger->ledger_client()->GetStringState(state::kWalletBrave),
            "BAD-DATA");
}

}  // namespace ledger
