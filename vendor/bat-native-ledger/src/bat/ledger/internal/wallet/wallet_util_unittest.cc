/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/wallet/wallet_util.h"
#include "bat/ledger/ledger.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=WalletUtilTest.*

namespace braveledger_wallet {

class WalletUtilTest : public testing::Test {
};

TEST(WalletUtilTest, GetWallet) {
  // no wallets
  std::map<std::string, ledger::ExternalWalletPtr> wallets;
  auto result = braveledger_wallet::GetWallet("brave", std::move(wallets));
  ASSERT_TRUE(!result);

  // different wallet
  auto diff = ledger::ExternalWallet::New();
  diff->address = "add1";
  wallets.insert(std::make_pair("different", std::move(diff)));
  result =
      braveledger_wallet::GetWallet(ledger::kWalletUphold, std::move(wallets));
  ASSERT_TRUE(!result);

  // uphold wallet
  auto uphold = ledger::ExternalWallet::New();
  uphold->address = "12355";
  wallets.insert(std::make_pair(ledger::kWalletUphold, std::move(uphold)));
  result =
      braveledger_wallet::GetWallet(ledger::kWalletUphold, std::move(wallets));
  ASSERT_EQ(result->address, "12355");
}

TEST(WalletUtilTest, ResetWallet) {
  // null wallet
  auto result = braveledger_wallet::ResetWallet(nullptr);
  ASSERT_TRUE(!result);

  // verfied wallet
  auto verified = ledger::ExternalWallet::New();
  verified->token = "1";
  verified->address = "2";
  verified->user_name = "3";
  verified->one_time_string = "4";
  verified->status = ledger::WalletStatus::VERIFIED;
  verified->transferred = true;

  result = braveledger_wallet::ResetWallet(std::move(verified));

  ASSERT_EQ(result->token, "");
  ASSERT_EQ(result->address, "");
  ASSERT_EQ(result->user_name, "");
  ASSERT_EQ(result->one_time_string, "");
  ASSERT_EQ(result->status, ledger::WalletStatus::DISCONNECTED_VERIFIED);
  ASSERT_EQ(result->transferred, true);

  // non verfied wallet
  auto non_verified = ledger::ExternalWallet::New();
  non_verified->status = ledger::WalletStatus::CONNECTED;
  result =
      braveledger_wallet::ResetWallet(std::move(non_verified));
  ASSERT_EQ(result->status, ledger::WalletStatus::DISCONNECTED_NOT_VERIFIED);
}


}  // namespace braveledger_wallet
