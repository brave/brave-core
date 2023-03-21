/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <map>
#include <utility>

#include "base/strings/strcat.h"
#include "base/strings/stringprintf.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/buildflags.h"
#include "brave/components/brave_rewards/core/common/random_util.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/ledger.h"
#include "brave/components/brave_rewards/core/ledger_client_mock.h"
#include "brave/components/brave_rewards/core/ledger_impl_mock.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"
#include "brave/components/brave_rewards/core/test/test_ledger_client.h"
#include "brave/components/brave_rewards/core/uphold/uphold_util.h"
#include "testing/gmock/include/gmock/gmock.h"

// npm run test -- brave_unit_tests --filter=UpholdUtilTest.*

using ::testing::_;

namespace ledger {
namespace uphold {

class UpholdUtilTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
  MockLedgerImpl mock_ledger_impl_;
};

TEST_F(UpholdUtilTest, GetClientId) {
  // production
  ledger::_environment = mojom::Environment::PRODUCTION;
  std::string result = uphold::GetClientId();
  ASSERT_EQ(result, BUILDFLAG(UPHOLD_CLIENT_ID));

  // staging
  ledger::_environment = mojom::Environment::STAGING;
  result = uphold::GetClientId();
  ASSERT_EQ(result, BUILDFLAG(UPHOLD_STAGING_CLIENT_ID));
}

TEST_F(UpholdUtilTest, GetClientSecret) {
  // production
  ledger::_environment = mojom::Environment::PRODUCTION;
  std::string result = uphold::GetClientSecret();
  ASSERT_EQ(result, BUILDFLAG(UPHOLD_CLIENT_SECRET));

  // staging
  ledger::_environment = mojom::Environment::STAGING;
  result = uphold::GetClientSecret();
  ASSERT_EQ(result, BUILDFLAG(UPHOLD_STAGING_CLIENT_SECRET));
}

TEST_F(UpholdUtilTest, GetFeeAddress) {
  // production
  ledger::_environment = mojom::Environment::PRODUCTION;
  std::string result = uphold::GetFeeAddress();
  ASSERT_EQ(result, kFeeAddressProduction);

  // staging
  ledger::_environment = mojom::Environment::STAGING;
  result = uphold::GetFeeAddress();
  ASSERT_EQ(result, kFeeAddressStaging);
}

TEST_F(UpholdUtilTest, GetLoginUrl) {
  // production
  ledger::_environment = mojom::Environment::PRODUCTION;
  std::string result = uphold::GetLoginUrl("rdfdsfsdfsdf");
  ASSERT_EQ(
      result,
      base::StrCat(
          {"https://uphold.com/authorize/", BUILDFLAG(UPHOLD_CLIENT_ID),
           "?scope=cards:read cards:write user:read "
           "transactions:read transactions:transfer:application "
           "transactions:transfer:others&intention=login&state=rdfdsfsdfsdf"}));

  // staging
  ledger::_environment = mojom::Environment::STAGING;
  result = uphold::GetLoginUrl("rdfdsfsdfsdf");
  ASSERT_EQ(
      result,
      base::StrCat(
          {"https://wallet-sandbox.uphold.com/authorize/",
           BUILDFLAG(UPHOLD_STAGING_CLIENT_ID),
           "?scope=cards:read cards:write user:read "
           "transactions:read transactions:transfer:application "
           "transactions:transfer:others&intention=login&state=rdfdsfsdfsdf"}));
}

TEST_F(UpholdUtilTest, GetActivityUrl) {
  // empty string
  std::string result = uphold::GetActivityUrl("");
  ASSERT_EQ(result, "");

  // production
  ledger::_environment = mojom::Environment::PRODUCTION;
  result = uphold::GetActivityUrl("9324i5i32459i");
  ASSERT_EQ(result,
            "https://uphold.com/dashboard/cards/9324i5i32459i/activity");

  // staging
  ledger::_environment = mojom::Environment::STAGING;
  result = uphold::GetActivityUrl("9324i5i32459i");
  ASSERT_EQ(result,
            "https://wallet-sandbox.uphold.com/dashboard/cards/9324i5i32459i/"
            "activity");
}

TEST_F(UpholdUtilTest, GetWallet) {
  // no wallet
  ON_CALL(*mock_ledger_impl_.mock_rewards_service(),
          GetStringState(state::kWalletUphold, _))
      .WillByDefault([](const std::string&, auto callback) {
        std::move(callback).Run("");
      });
  auto result = mock_ledger_impl_.uphold()->GetWallet();
  ASSERT_TRUE(!result);

  ON_CALL(*mock_ledger_impl_.mock_rewards_service(),
          GetStringState(state::kWalletUphold, _))
      .WillByDefault([](const std::string&, auto callback) {
        std::string wallet = FakeEncryption::Base64EncryptString(R"({
          "account_url":"https://wallet-sandbox.uphold.com/dashboard",
          "address":"2323dff2ba-d0d1-4dfw-8e56-a2605bcaf4af",
          "fees":{},
          "login_url":"https://wallet-sandbox.uphold.com/authorize/4c2b665ca060d",
          "one_time_string":"1F747AE0A708E47ED7E650BF1856B5A4EF7E36833BDB115",
          "status":2,
          "token":"4c80232r219c30cdf112208890a32c7e00",
          "user_name":"test"
        })");
        std::move(callback).Run(std::move(wallet));
      });

  // uphold wallet
  result = mock_ledger_impl_.uphold()->GetWallet();
  ASSERT_TRUE(result);
  ASSERT_EQ(result->address, "2323dff2ba-d0d1-4dfw-8e56-a2605bcaf4af");
  ASSERT_EQ(result->user_name, "test");
  ASSERT_EQ(result->token, "4c80232r219c30cdf112208890a32c7e00");
  ASSERT_EQ(result->status, mojom::WalletStatus::kConnected);
}

TEST_F(UpholdUtilTest, GenerateRandomHexString) {
  // string for testing
  ledger::is_testing = true;
  auto result = ledger::util::GenerateRandomHexString();
  ASSERT_EQ(result, "123456789");

  // random string
  ledger::is_testing = false;
  ledger::_environment = mojom::Environment::STAGING;
  result = ledger::util::GenerateRandomHexString();
  ASSERT_EQ(result.length(), 64u);
}

TEST_F(UpholdUtilTest, GenerateLinks) {
  ledger::_environment = mojom::Environment::STAGING;

  auto wallet = mojom::ExternalWallet::New();

  // Not connected
  wallet->status = mojom::WalletStatus::kNotConnected;
  wallet->token = "";    // must be empty
  wallet->address = "";  // must be empty
  auto result = uphold::GenerateLinks(wallet->Clone());
  ASSERT_EQ(
      result->login_url,
      base::StrCat({"https://wallet-sandbox.uphold.com/authorize/",
                    BUILDFLAG(UPHOLD_STAGING_CLIENT_ID),
                    "?scope=cards:read cards:write user:read "
                    "transactions:read transactions:transfer:application "
                    "transactions:transfer:others&intention=login&state="}));
  ASSERT_EQ(result->account_url, "https://wallet-sandbox.uphold.com/dashboard");

  // Connected
  wallet->status = mojom::WalletStatus::kConnected;
  wallet->token = "must be non-empty";
  wallet->address = "123123123124234234234";  // must be non-empty
  result = uphold::GenerateLinks(wallet->Clone());
  ASSERT_EQ(
      result->login_url,
      base::StrCat({"https://wallet-sandbox.uphold.com/authorize/",
                    BUILDFLAG(UPHOLD_STAGING_CLIENT_ID),
                    "?scope=cards:read cards:write user:read "
                    "transactions:read transactions:transfer:application "
                    "transactions:transfer:others&intention=login&state="}));
  ASSERT_EQ(result->account_url, "https://wallet-sandbox.uphold.com/dashboard");

  // Logged out
  wallet->status = mojom::WalletStatus::kLoggedOut;
  wallet->token = "";    // must be empty
  wallet->address = "";  // must be empty
  result = uphold::GenerateLinks(wallet->Clone());
  ASSERT_EQ(
      result->login_url,
      base::StrCat({"https://wallet-sandbox.uphold.com/authorize/",
                    BUILDFLAG(UPHOLD_STAGING_CLIENT_ID),
                    "?scope=cards:read cards:write user:read "
                    "transactions:read transactions:transfer:application "
                    "transactions:transfer:others&intention=login&state="}));
  ASSERT_EQ(result->account_url, "https://wallet-sandbox.uphold.com/dashboard");
}

}  // namespace uphold
}  // namespace ledger
