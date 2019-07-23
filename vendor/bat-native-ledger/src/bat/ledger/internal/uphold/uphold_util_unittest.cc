/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "bat/ledger/internal/uphold/uphold_util.h"
#include "bat/ledger/ledger.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=UpholdUtilTest.*

namespace braveledger_uphold {

class UpholdUtilTest : public testing::Test {
};

TEST(UpholdUtilTest, GetClientId) {
  // production
  ledger::is_production = true;
  std::string result = braveledger_uphold::GetClientId();
  ASSERT_EQ(result, kClientIdProduction);

  // staging
  ledger::is_production = false;
  result = braveledger_uphold::GetClientId();
  ASSERT_EQ(result, kClientIdStaging);
}

TEST(UpholdUtilTest, GetClientSecret) {
  // production
  ledger::is_production = true;
  std::string result = braveledger_uphold::GetClientSecret();
  ASSERT_EQ(result, kClientSecretProduction);

  // staging
  ledger::is_production = false;
  result = braveledger_uphold::GetClientSecret();
  ASSERT_EQ(result, kClientSecretStaging);
}

TEST(UpholdUtilTest, GetAPIUrl) {
  // empty path, production
  ledger::is_production = true;
  std::string result = braveledger_uphold::GetAPIUrl("");
  ASSERT_EQ(result, kAPIUrlProduction);

  // empty path, staging
  ledger::is_production = false;
  result = braveledger_uphold::GetAPIUrl("");
  ASSERT_EQ(result, kAPIUrlStaging);

  // with path
  ledger::is_production = false;
  result = braveledger_uphold::GetAPIUrl("/v0/testing");
  ASSERT_EQ(result, static_cast<std::string>(kAPIUrlStaging) + "/v0/testing");
}

TEST(UpholdUtilTest, GetFeeAddress) {
  // production
  ledger::is_production = true;
  std::string result = braveledger_uphold::GetFeeAddress();
  ASSERT_EQ(result, kFeeAddressProduction);

  // staging
  ledger::is_production = false;
  result = braveledger_uphold::GetFeeAddress();
  ASSERT_EQ(result, kFeeAddressStaging);
}

TEST(UpholdUtilTest, ConvertToProbi) {
  // empty string
  std::string result = braveledger_uphold::ConvertToProbi("");
  ASSERT_EQ(result, "0");

  // single dig int
  result = braveledger_uphold::ConvertToProbi("5");
  ASSERT_EQ(result, "5000000000000000000");

  // two dig int
  result = braveledger_uphold::ConvertToProbi("15");
  ASSERT_EQ(result, "15000000000000000000");

  // single dig decimal
  result = braveledger_uphold::ConvertToProbi("5.4");
  ASSERT_EQ(result, "5400000000000000000");

  // two dig decimal
  result = braveledger_uphold::ConvertToProbi("5.45");
  ASSERT_EQ(result, "5450000000000000000");
}

TEST(UpholdUtilTest, GetVerifyUrl) {
  // production
  ledger::is_production = true;
  std::string result = braveledger_uphold::GetVerifyUrl("rdfdsfsdfsdf");
  ASSERT_EQ(result,
      "https://uphold.com/authorize/"
      "6d8d9473ed20be627f71ed46e207f40c004c5b1a?scope=cards:read cards:write "
      "user:read transactions:read transactions:transfer:application "
      "transactions:transfer:others&intention=kyc&state=rdfdsfsdfsdf");

  // staging
  ledger::is_production = false;
  result = braveledger_uphold::GetVerifyUrl("rdfdsfsdfsdf");
  ASSERT_EQ(result,
      "https://sandbox.uphold.com/authorize/"
      "4c2b665ca060d912fec5c735c734859a06118cc8?scope=cards:read cards:write "
      "user:read transactions:read transactions:transfer:application "
      "transactions:transfer:others&intention=kyc&state=rdfdsfsdfsdf");
}

TEST(UpholdUtilTest, GetAddUrl) {
  // empty string
  std::string result = braveledger_uphold::GetAddUrl("");
  ASSERT_EQ(result, "");

  // production
  ledger::is_production = true;
  result = braveledger_uphold::GetAddUrl("9324i5i32459i");
  ASSERT_EQ(result, "https://uphold.com/dashboard/cards/9324i5i32459i/add");

  // staging
  ledger::is_production = false;
  result = braveledger_uphold::GetAddUrl("9324i5i32459i");
  ASSERT_EQ(result,
      "https://sandbox.uphold.com/dashboard/cards/9324i5i32459i/add");
}

TEST(UpholdUtilTest, GetWithdrawUrl) {
  // empty string
  std::string result = braveledger_uphold::GetWithdrawUrl("");
  ASSERT_EQ(result, "");

  // production
  ledger::is_production = true;
  result = braveledger_uphold::GetWithdrawUrl("9324i5i32459i");
  ASSERT_EQ(result, "https://uphold.com/dashboard/cards/9324i5i32459i/use");

  // staging
  ledger::is_production = false;
  result = braveledger_uphold::GetWithdrawUrl("9324i5i32459i");
  ASSERT_EQ(result,
      "https://sandbox.uphold.com/dashboard/cards/9324i5i32459i/use");
}

TEST(UpholdUtilTest, GetSecondStepVerify) {
  // production
  ledger::is_production = true;
  std::string result = braveledger_uphold::GetSecondStepVerify();
  ASSERT_EQ(result,
      "https://uphold.com/signup/step2?"
      "application_id=6d8d9473ed20be627f71ed46e207f40c004c5b1a&intention=kyc");

  // staging
  ledger::is_production = false;
  result = braveledger_uphold::GetSecondStepVerify();
  ASSERT_EQ(result,
      "https://sandbox.uphold.com/signup/step2?"
      "application_id=4c2b665ca060d912fec5c735c734859a06118cc8&intention=kyc");
}

TEST(UpholdUtilTest, GetWallet) {
  // no wallets
  std::map<std::string, ledger::ExternalWalletPtr> wallets;
  ledger::ExternalWalletPtr result =
      braveledger_uphold::GetWallet(std::move(wallets));
  ASSERT_TRUE(!result);

  // different wallet
  auto diff = ledger::ExternalWallet::New();
  diff->address = "add1";
  wallets.insert(std::make_pair("different", std::move(diff)));
  result = braveledger_uphold::GetWallet(std::move(wallets));
  ASSERT_TRUE(!result);

  // uphold wallet
  auto uphold = ledger::ExternalWallet::New();
  uphold->address = "12355";
  wallets.insert(std::make_pair(ledger::kWalletUphold, std::move(uphold)));
  result = braveledger_uphold::GetWallet(std::move(wallets));
  ASSERT_EQ(result->address, "12355");
}

TEST(UpholdUtilTest, RequestAuthorization) {
  // token is defined
  auto result = braveledger_uphold::RequestAuthorization("2423423424");
  ASSERT_EQ(result.at(0),
      "Authorization: Bearer 2423423424");


  // token is not defined
  ledger::is_production = false;
  result = braveledger_uphold::RequestAuthorization();
  ASSERT_EQ(result.at(0),
      "Authorization: Basic NGMyYjY2NWNhMDYwZDkxMmZlYzVjNzM1YzczNDg1OWEwNjEx"
      "OGNjODo2N2JmODdkYTA5Njc0OGM1YmMxZTE5NWNmYmRkNTlkYjAwNjYxOGEw");
}

TEST(UpholdUtilTest, GenerateRandomString) {
  // string for testing
  auto result = braveledger_uphold::GenerateRandomString(true);
  ASSERT_EQ(result, "123456789");

  // random string
  ledger::is_production = false;
  result = braveledger_uphold::GenerateRandomString(false);
  ASSERT_EQ(result.length(), 64u);
}

}  // namespace braveledger_uphold
