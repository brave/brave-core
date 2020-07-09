/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/uphold/uphold_util.h"
#include "bat/ledger/ledger.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=UpholdUtilTest.*

namespace braveledger_uphold {

class UpholdUtilTest : public testing::Test {
};

TEST(UpholdUtilTest, GetClientId) {
  // production
  ledger::_environment = ledger::Environment::PRODUCTION;
  std::string result = braveledger_uphold::GetClientId();
  ASSERT_EQ(result, kClientIdProduction);

  // staging
  ledger::_environment = ledger::Environment::STAGING;
  result = braveledger_uphold::GetClientId();
  ASSERT_EQ(result, kClientIdStaging);
}

TEST(UpholdUtilTest, GetClientSecret) {
  // production
  ledger::_environment = ledger::Environment::PRODUCTION;
  std::string result = braveledger_uphold::GetClientSecret();
  ASSERT_EQ(result, kClientSecretProduction);

  // staging
  ledger::_environment = ledger::Environment::STAGING;
  result = braveledger_uphold::GetClientSecret();
  ASSERT_EQ(result, kClientSecretStaging);
}

TEST(UpholdUtilTest, GetAPIUrl) {
  // empty path, production
  ledger::_environment = ledger::Environment::PRODUCTION;
  std::string result = braveledger_uphold::GetAPIUrl("");
  ASSERT_EQ(result, kAPIUrlProduction);

  // empty path, staging
  ledger::_environment = ledger::Environment::STAGING;
  result = braveledger_uphold::GetAPIUrl("");
  ASSERT_EQ(result, kAPIUrlStaging);

  // with path
  ledger::_environment = ledger::Environment::STAGING;
  result = braveledger_uphold::GetAPIUrl("/v0/testing");
  ASSERT_EQ(result, static_cast<std::string>(kAPIUrlStaging) + "/v0/testing");
}

TEST(UpholdUtilTest, GetFeeAddress) {
  // production
  ledger::_environment = ledger::Environment::PRODUCTION;
  std::string result = braveledger_uphold::GetFeeAddress();
  ASSERT_EQ(result, kFeeAddressProduction);

  // staging
  ledger::_environment = ledger::Environment::STAGING;
  result = braveledger_uphold::GetFeeAddress();
  ASSERT_EQ(result, kFeeAddressStaging);
}

TEST(UpholdUtilTest, GetAuthorizeUrl) {
  // production
  ledger::_environment = ledger::Environment::PRODUCTION;
  std::string result =
      braveledger_uphold::GetAuthorizeUrl("rdfdsfsdfsdf", true);
  ASSERT_EQ(result,
      "https://uphold.com/authorize/"
      "6d8d9473ed20be627f71ed46e207f40c004c5b1a?scope=accounts:read "
      "accounts:write cards:read cards:write user:read "
      "transactions:deposit transactions:read "
      "transactions:transfer:application transactions:transfer:others"
      "&intention=kyc&state=rdfdsfsdfsdf");

  // staging
  ledger::_environment = ledger::Environment::STAGING;
  result = braveledger_uphold::GetAuthorizeUrl("rdfdsfsdfsdf", true);
  ASSERT_EQ(result,
      "https://sandbox.uphold.com/authorize/"
      "4c2b665ca060d912fec5c735c734859a06118cc8?scope=accounts:read "
      "accounts:write cards:read cards:write user:read "
      "transactions:deposit transactions:read "
      "transactions:transfer:application transactions:transfer:others"
      "&intention=kyc&state=rdfdsfsdfsdf");
  // production
  ledger::_environment = ledger::Environment::PRODUCTION;
  result =
      braveledger_uphold::GetAuthorizeUrl("rdfdsfsdfsdf", false);
  ASSERT_EQ(result,
      "https://uphold.com/authorize/"
      "6d8d9473ed20be627f71ed46e207f40c004c5b1a?scope=accounts:read "
      "accounts:write cards:read cards:write user:read "
      "transactions:deposit transactions:read "
      "transactions:transfer:application transactions:transfer:others"
      "&intention=login&state=rdfdsfsdfsdf");

  // staging
  ledger::_environment = ledger::Environment::STAGING;
  result = braveledger_uphold::GetAuthorizeUrl("rdfdsfsdfsdf", false);
  ASSERT_EQ(result,
      "https://sandbox.uphold.com/authorize/"
      "4c2b665ca060d912fec5c735c734859a06118cc8?scope=accounts:read "
      "accounts:write cards:read cards:write user:read "
      "transactions:deposit transactions:read "
      "transactions:transfer:application transactions:transfer:others"
      "&intention=login&state=rdfdsfsdfsdf");
}

TEST(UpholdUtilTest, GetAddUrl) {
  // empty string
  std::string result = braveledger_uphold::GetAddUrl("");
  ASSERT_EQ(result, "");

  // production
  ledger::_environment = ledger::Environment::PRODUCTION;
  result = braveledger_uphold::GetAddUrl("9324i5i32459i");
  ASSERT_EQ(result, "https://uphold.com/dashboard/cards/9324i5i32459i/add");

  // staging
  ledger::_environment = ledger::Environment::STAGING;
  result = braveledger_uphold::GetAddUrl("9324i5i32459i");
  ASSERT_EQ(result,
      "https://sandbox.uphold.com/dashboard/cards/9324i5i32459i/add");
}

TEST(UpholdUtilTest, GetWithdrawUrl) {
  // empty string
  std::string result = braveledger_uphold::GetWithdrawUrl("");
  ASSERT_EQ(result, "");

  // production
  ledger::_environment = ledger::Environment::PRODUCTION;
  result = braveledger_uphold::GetWithdrawUrl("9324i5i32459i");
  ASSERT_EQ(result, "https://uphold.com/dashboard/cards/9324i5i32459i/use");

  // staging
  ledger::_environment = ledger::Environment::STAGING;
  result = braveledger_uphold::GetWithdrawUrl("9324i5i32459i");
  ASSERT_EQ(result,
      "https://sandbox.uphold.com/dashboard/cards/9324i5i32459i/use");
}

TEST(UpholdUtilTest, GetSecondStepVerify) {
  // production
  ledger::_environment = ledger::Environment::PRODUCTION;
  std::string result = braveledger_uphold::GetSecondStepVerify();
  ASSERT_EQ(result,
      "https://uphold.com/signup/step2?"
      "application_id=6d8d9473ed20be627f71ed46e207f40c004c5b1a&intention=kyc");

  // staging
  ledger::_environment = ledger::Environment::STAGING;
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
  ledger::_environment = ledger::Environment::STAGING;
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
  ledger::_environment = ledger::Environment::STAGING;
  result = braveledger_uphold::GenerateRandomString(false);
  ASSERT_EQ(result.length(), 64u);
}

TEST(UpholdUtilTest, GenerateLinks) {
  ledger::_environment = ledger::Environment::STAGING;

  auto wallet = ledger::ExternalWallet::New();
  wallet->address = "123123123124234234234";

  // Not connected
  wallet->status = ledger::WalletStatus::NOT_CONNECTED;
  auto result = braveledger_uphold::GenerateLinks(wallet->Clone());
  ASSERT_EQ(result->add_url, "");
  ASSERT_EQ(result->withdraw_url, "");
  ASSERT_EQ(result->verify_url,
      "https://sandbox.uphold.com/authorize/4c2b665ca060d912fec5c735c734859a06"
      "118cc8?scope=accounts:read "
      "accounts:write cards:read cards:write user:read "
      "transactions:deposit transactions:read "
      "transactions:transfer:application transactions:transfer:others"
      "&intention=kyc&state=");
  ASSERT_EQ(result->account_url, "https://sandbox.uphold.com/dashboard");

  // Connected
  wallet->status = ledger::WalletStatus::CONNECTED;
  result = braveledger_uphold::GenerateLinks(wallet->Clone());
  ASSERT_EQ(result->add_url,
      "https://sandbox.uphold.com/dashboard/cards/123123123124234234234/add");
  ASSERT_EQ(result->withdraw_url,
      "https://sandbox.uphold.com/signup/step2?application_id="
      "4c2b665ca060d912fec5c735c734859a06118cc8&intention=kyc");
  ASSERT_EQ(result->verify_url,
      "https://sandbox.uphold.com/signup/step2?application_id="
      "4c2b665ca060d912fec5c735c734859a06118cc8&intention=kyc");
  ASSERT_EQ(result->account_url, "https://sandbox.uphold.com/dashboard");

  // Verified
  wallet->status = ledger::WalletStatus::VERIFIED;
  result = braveledger_uphold::GenerateLinks(wallet->Clone());
  ASSERT_EQ(result->add_url,
      "https://sandbox.uphold.com/dashboard/cards/123123123124234234234/add");
  ASSERT_EQ(result->withdraw_url,
      "https://sandbox.uphold.com/dashboard/cards/123123123124234234234/use");
  ASSERT_EQ(result->verify_url, "");
  ASSERT_EQ(result->account_url, "https://sandbox.uphold.com/dashboard");

  // Disconnected Non-Verified
  wallet->status = ledger::WalletStatus::DISCONNECTED_NOT_VERIFIED;
  result = braveledger_uphold::GenerateLinks(wallet->Clone());
  ASSERT_EQ(result->add_url, "");
  ASSERT_EQ(result->withdraw_url, "");
  ASSERT_EQ(result->verify_url,
      "https://sandbox.uphold.com/authorize/4c2b665ca060d912fec5c735c734859a0"
      "6118cc8?scope=accounts:read "
      "accounts:write cards:read cards:write user:read "
      "transactions:deposit transactions:read "
      "transactions:transfer:application transactions:transfer:others"
      "&intention=kyc&state=");
  ASSERT_EQ(result->account_url, "https://sandbox.uphold.com/dashboard");

  // Disconnected Verified
  wallet->status = ledger::WalletStatus::DISCONNECTED_VERIFIED;
  result = braveledger_uphold::GenerateLinks(wallet->Clone());
  ASSERT_EQ(result->add_url, "");
  ASSERT_EQ(result->withdraw_url, "");
  ASSERT_EQ(result->verify_url,
      "https://sandbox.uphold.com/authorize/4c2b665ca060d912fec5c735c734859a0"
      "6118cc8?scope=accounts:read "
      "accounts:write cards:read cards:write user:read "
      "transactions:deposit transactions:read "
      "transactions:transfer:application transactions:transfer:others"
      "&intention=kyc&state=");
  ASSERT_EQ(result->account_url, "https://sandbox.uphold.com/dashboard");

  // Pending
  wallet->status = ledger::WalletStatus::PENDING;
  result = braveledger_uphold::GenerateLinks(wallet->Clone());
  ASSERT_EQ(result->add_url,
          "https://sandbox.uphold.com/signup/step2?"
          "application_id=4c2b665ca060d912fec5c735c734859a06118cc8&"
          "intention=kyc");
  ASSERT_EQ(result->withdraw_url,
          "https://sandbox.uphold.com/signup/step2?"
          "application_id=4c2b665ca060d912fec5c735c734859a06118cc8&"
          "intention=kyc");
  ASSERT_EQ(result->verify_url,
          "https://sandbox.uphold.com/signup/step2?"
          "application_id=4c2b665ca060d912fec5c735c734859a06118cc8&"
          "intention=kyc");
  ASSERT_EQ(result->account_url, "https://sandbox.uphold.com/dashboard");
}

TEST(UpholdUtilTest, GenerateVerifyLink) {
  ledger::_environment = ledger::Environment::STAGING;

  auto wallet = ledger::ExternalWallet::New();
  wallet->one_time_string = "123123123124234234234";

  // Not connected
  wallet->status = ledger::WalletStatus::NOT_CONNECTED;
  auto result = braveledger_uphold::GenerateVerifyLink(wallet->Clone());
  ASSERT_EQ(result,
      "https://sandbox.uphold.com/authorize/4c2b665ca060d912fec5c735c734859a06"
      "118cc8?scope=accounts:read "
      "accounts:write cards:read cards:write user:read "
      "transactions:deposit transactions:read "
      "transactions:transfer:application transactions:transfer:others"
      "&intention=kyc&state=123123123124234234234");

  // Connected
  wallet->status = ledger::WalletStatus::CONNECTED;
  result = braveledger_uphold::GenerateVerifyLink(wallet->Clone());
  ASSERT_EQ(result,
      "https://sandbox.uphold.com/signup/step2?application_id="
      "4c2b665ca060d912fec5c735c734859a06118cc8&intention=kyc");

  // Verified
  wallet->status = ledger::WalletStatus::VERIFIED;
  result = braveledger_uphold::GenerateVerifyLink(wallet->Clone());
  ASSERT_EQ(result, "");

  // Disconnected Non-Verified
  wallet->status = ledger::WalletStatus::DISCONNECTED_NOT_VERIFIED;
  result = braveledger_uphold::GenerateVerifyLink(wallet->Clone());
  ASSERT_EQ(result,
      "https://sandbox.uphold.com/authorize/4c2b665ca060d912fec5c735c734859a06"
      "118cc8?scope=accounts:read "
      "accounts:write cards:read cards:write user:read "
      "transactions:deposit transactions:read "
      "transactions:transfer:application transactions:transfer:others"
      "&intention=kyc&state=123123123124234234234");

  // Disconnected Verified
  wallet->status = ledger::WalletStatus::DISCONNECTED_VERIFIED;
  result = braveledger_uphold::GenerateVerifyLink(wallet->Clone());
  ASSERT_EQ(result,
      "https://sandbox.uphold.com/authorize/4c2b665ca060d912fec5c735c734859a06"
      "118cc8?scope=accounts:read "
      "accounts:write cards:read cards:write user:read "
      "transactions:deposit transactions:read "
      "transactions:transfer:application transactions:transfer:others"
      "&intention=kyc&state=123123123124234234234");

  // Pending
  wallet->status = ledger::WalletStatus::PENDING;
  result = braveledger_uphold::GenerateVerifyLink(wallet->Clone());
  ASSERT_EQ(result,
          "https://sandbox.uphold.com/signup/step2?"
          "application_id=4c2b665ca060d912fec5c735c734859a06118cc8&"
          "intention=kyc");
}

}  // namespace braveledger_uphold
