/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/wallet/wallet_util.h"

#include "bat/ledger/internal/core/bat_ledger_test.h"
#include "bat/ledger/mojom_structs.h"

// npm run test -- brave_unit_tests --filter=WalletUtilTest.*

namespace ledger {
namespace wallet {

class WalletUtilTest : public BATLedgerTest {};

TEST_F(WalletUtilTest, InvalidJSON) {
  const char data[] = "";
  type::ExternalWalletPtr wallet = ExternalWalletPtrFromJSON(data, "uphold");
  EXPECT_EQ(nullptr, wallet.get());
}

TEST_F(WalletUtilTest, ExternalWalletPtrFromJSON) {
  const char data[] =
      "{\n"
      "  \"token\": \"sI5rKiy6ijzbbJgE2MMFzAbTc6udYYXEi3wzS9iknP6n\",\n"
      "  \"address\": \"6a752063-8958-44d5-b5db-71543f18567d\",\n"
      "  \"one_time_string\": \"eda4c873eac72e1ecc30e77b25bb623b8b5bf99f\",\n"
      "  \"status\": 2,\n"
      "  \"user_name\": \"random_user\",\n"
      "  \"verify_url\": \"https://random.domain/verify\","
      "  \"add_url\": \"https://random.domain/add\","
      "  \"withdraw_url\": \"https://random.domain/withdraw\","
      "  \"account_url\": \"https://random.domain/account\","
      "  \"login_url\": \"https://random.domain/login\","
      "  \"fees\": {\"brave\": 5.00}"
      "}\n";

  type::ExternalWalletPtr wallet = ExternalWalletPtrFromJSON(data, "uphold");
  EXPECT_EQ(wallet->token, "sI5rKiy6ijzbbJgE2MMFzAbTc6udYYXEi3wzS9iknP6n");
  EXPECT_EQ(wallet->address, "6a752063-8958-44d5-b5db-71543f18567d");
  EXPECT_EQ(wallet->one_time_string,
            "eda4c873eac72e1ecc30e77b25bb623b8b5bf99f");
  EXPECT_EQ(wallet->status, ledger::type::WalletStatus::VERIFIED);
  EXPECT_EQ(wallet->user_name, "random_user");
  EXPECT_EQ(wallet->verify_url, "https://random.domain/verify");
  EXPECT_EQ(wallet->add_url, "https://random.domain/add");
  EXPECT_EQ(wallet->withdraw_url, "https://random.domain/withdraw");
  EXPECT_EQ(wallet->account_url, "https://random.domain/account");
  EXPECT_EQ(wallet->login_url, "https://random.domain/login");
  EXPECT_NE(wallet->fees.find("brave"), wallet->fees.end());
  EXPECT_EQ(wallet->fees["brave"], 5.00);
}

}  // namespace wallet
}  // namespace ledger
