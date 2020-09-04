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

namespace ledger {
namespace wallet {

class WalletUtilTest : public testing::Test {
};

TEST(WalletUtilTest, GetWallet) {
  // no wallets
  std::map<std::string, type::ExternalWalletPtr> wallets;
  auto result = wallet::GetWallet("brave", std::move(wallets));
  ASSERT_TRUE(!result);

  // different wallet
  auto diff = type::ExternalWallet::New();
  diff->address = "add1";
  wallets.insert(std::make_pair("different", std::move(diff)));
  result = wallet::GetWallet(constant::kWalletUphold, std::move(wallets));
  ASSERT_TRUE(!result);

  // uphold wallet
  auto uphold = type::ExternalWallet::New();
  uphold->address = "12355";
  wallets.insert(std::make_pair(constant::kWalletUphold, std::move(uphold)));
  result = wallet::GetWallet(constant::kWalletUphold, std::move(wallets));
  ASSERT_EQ(result->address, "12355");
}

TEST(WalletUtilTest, ResetWalletNull) {
  auto result = wallet::ResetWallet(nullptr);
  ASSERT_TRUE(!result);
}

TEST(WalletUtilTest, ResetWalletVerifiedWallet) {
  auto wallet = type::ExternalWallet::New();
  wallet->token = "1";
  wallet->address = "2";
  wallet->user_name = "3";
  wallet->one_time_string = "4";
  wallet->status = type::WalletStatus::VERIFIED;

  auto reset_wallet = wallet::ResetWallet(std::move(wallet));

  type::ExternalWallet expected_wallet;
  expected_wallet.status = type::WalletStatus::DISCONNECTED_VERIFIED;

  ASSERT_TRUE(expected_wallet.Equals(*reset_wallet));
}

TEST(WalletUtilTest, ResetWalletNotVerifiedWallet) {
  auto not_verified = type::ExternalWallet::New();
  not_verified->status = type::WalletStatus::CONNECTED;
  auto reset_wallet =
      wallet::ResetWallet(std::move(not_verified));

  type::ExternalWallet expected_wallet;
  expected_wallet.status = type::WalletStatus::DISCONNECTED_NOT_VERIFIED;

  ASSERT_TRUE(expected_wallet.Equals(*reset_wallet));
}

}  // namespace wallet
}  // namespace ledger
