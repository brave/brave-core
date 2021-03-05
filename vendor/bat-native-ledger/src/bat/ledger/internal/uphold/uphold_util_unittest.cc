/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <memory>
#include <utility>

#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/uphold/uphold_util.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "bat/ledger/ledger.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "base/test/task_environment.h"

// npm run test -- brave_unit_tests --filter=UpholdUtilTest.*

using ::testing::_;

namespace ledger {
namespace uphold {

class UpholdUtilTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<ledger::MockLedgerImpl> mock_ledger_impl_;

  UpholdUtilTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<ledger::MockLedgerImpl>(mock_ledger_client_.get());
  }

  ~UpholdUtilTest() override {}
};

TEST_F(UpholdUtilTest, GetClientId) {
  // production
  ledger::_environment = type::Environment::PRODUCTION;
  std::string result = uphold::GetClientId();
  ASSERT_EQ(result, kClientIdProduction);

  // staging
  ledger::_environment = type::Environment::STAGING;
  result = uphold::GetClientId();
  ASSERT_EQ(result, kClientIdStaging);
}

TEST_F(UpholdUtilTest, GetFeeAddress) {
  // production
  ledger::_environment = type::Environment::PRODUCTION;
  std::string result = uphold::GetFeeAddress();
  ASSERT_EQ(result, kFeeAddressProduction);

  // staging
  ledger::_environment = type::Environment::STAGING;
  result = uphold::GetFeeAddress();
  ASSERT_EQ(result, kFeeAddressStaging);
}

TEST_F(UpholdUtilTest, GetAuthorizeUrl) {
  // production
  ledger::_environment = type::Environment::PRODUCTION;
  std::string result =
      uphold::GetAuthorizeUrl("rdfdsfsdfsdf", true);
  ASSERT_EQ(result,
      "https://uphold.com/authorize/"
      "6d8d9473ed20be627f71ed46e207f40c004c5b1a?scope=accounts:read "
      "accounts:write cards:read cards:write user:read "
      "transactions:deposit transactions:read "
      "transactions:transfer:application transactions:transfer:others"
      "&intention=kyc&state=rdfdsfsdfsdf");

  // staging
  ledger::_environment = type::Environment::STAGING;
  result = uphold::GetAuthorizeUrl("rdfdsfsdfsdf", true);
  ASSERT_EQ(result,
      "https://sandbox.uphold.com/authorize/"
      "4c2b665ca060d912fec5c735c734859a06118cc8?scope=accounts:read "
      "accounts:write cards:read cards:write user:read "
      "transactions:deposit transactions:read "
      "transactions:transfer:application transactions:transfer:others"
      "&intention=kyc&state=rdfdsfsdfsdf");
  // production
  ledger::_environment = type::Environment::PRODUCTION;
  result =
      uphold::GetAuthorizeUrl("rdfdsfsdfsdf", false);
  ASSERT_EQ(result,
      "https://uphold.com/authorize/"
      "6d8d9473ed20be627f71ed46e207f40c004c5b1a?scope=accounts:read "
      "accounts:write cards:read cards:write user:read "
      "transactions:deposit transactions:read "
      "transactions:transfer:application transactions:transfer:others"
      "&intention=login&state=rdfdsfsdfsdf");

  // staging
  ledger::_environment = type::Environment::STAGING;
  result = uphold::GetAuthorizeUrl("rdfdsfsdfsdf", false);
  ASSERT_EQ(result,
      "https://sandbox.uphold.com/authorize/"
      "4c2b665ca060d912fec5c735c734859a06118cc8?scope=accounts:read "
      "accounts:write cards:read cards:write user:read "
      "transactions:deposit transactions:read "
      "transactions:transfer:application transactions:transfer:others"
      "&intention=login&state=rdfdsfsdfsdf");
}

TEST_F(UpholdUtilTest, GetAddUrl) {
  // empty string
  std::string result = uphold::GetAddUrl("");
  ASSERT_EQ(result, "");

  // production
  ledger::_environment = type::Environment::PRODUCTION;
  result = uphold::GetAddUrl("9324i5i32459i");
  ASSERT_EQ(result, "https://uphold.com/dashboard/cards/9324i5i32459i/add");

  // staging
  ledger::_environment = type::Environment::STAGING;
  result = uphold::GetAddUrl("9324i5i32459i");
  ASSERT_EQ(result,
      "https://sandbox.uphold.com/dashboard/cards/9324i5i32459i/add");
}

TEST_F(UpholdUtilTest, GetWithdrawUrl) {
  // empty string
  std::string result = uphold::GetWithdrawUrl("");
  ASSERT_EQ(result, "");

  // production
  ledger::_environment = type::Environment::PRODUCTION;
  result = uphold::GetWithdrawUrl("9324i5i32459i");
  ASSERT_EQ(result, "https://uphold.com/dashboard/cards/9324i5i32459i/use");

  // staging
  ledger::_environment = type::Environment::STAGING;
  result = uphold::GetWithdrawUrl("9324i5i32459i");
  ASSERT_EQ(result,
      "https://sandbox.uphold.com/dashboard/cards/9324i5i32459i/use");
}

TEST_F(UpholdUtilTest, GetSecondStepVerify) {
  // production
  ledger::_environment = type::Environment::PRODUCTION;
  std::string result = uphold::GetSecondStepVerify();
  ASSERT_EQ(result,
      "https://uphold.com/signup/step2?"
      "application_id=6d8d9473ed20be627f71ed46e207f40c004c5b1a&intention=kyc");

  // staging
  ledger::_environment = type::Environment::STAGING;
  result = uphold::GetSecondStepVerify();
  ASSERT_EQ(result,
      "https://sandbox.uphold.com/signup/step2?"
      "application_id=4c2b665ca060d912fec5c735c734859a06118cc8&intention=kyc");
}

TEST_F(UpholdUtilTest, GetWallet) {
  // no wallet
  ON_CALL(*mock_ledger_client_, GetEncryptedStringState(state::kWalletUphold))
      .WillByDefault(testing::Return(""));
  auto result = uphold::GetWallet(mock_ledger_impl_.get());
  ASSERT_TRUE(!result);

  const std::string wallet =R"({
    "account_url":"https://sandbox.uphold.com/dashboard",
    "add_url":"https://sandbox.uphold.com/dashboard/cards/asadasdasd/add",
    "address":"2323dff2ba-d0d1-4dfw-8e56-a2605bcaf4af",
    "fees":{},
    "login_url":"https://sandbox.uphold.com/authorize/4c2b665ca060d",
    "one_time_string":"1F747AE0A708E47ED7E650BF1856B5A4EF7E36833BDB1158A108F8",
    "status":2,
    "token":"4c80232r219c30cdf112208890a32c7e00",
    "user_name":"test",
    "verify_url":"",
    "withdraw_url":"https://sandbox.uphold.com/dashboard/cards/asadasdasd/use"
  })";

  ON_CALL(*mock_ledger_client_, GetEncryptedStringState(state::kWalletUphold))
      .WillByDefault(testing::Return(wallet));

  // uphold wallet
  result = uphold::GetWallet(mock_ledger_impl_.get());
  ASSERT_TRUE(result);
  ASSERT_EQ(result->address, "2323dff2ba-d0d1-4dfw-8e56-a2605bcaf4af");
  ASSERT_EQ(result->user_name, "test");
  ASSERT_EQ(result->token, "4c80232r219c30cdf112208890a32c7e00");
  ASSERT_EQ(result->status, type::WalletStatus::VERIFIED);
}

TEST_F(UpholdUtilTest, GenerateRandomString) {
  // string for testing
  auto result = uphold::GenerateRandomString(true);
  ASSERT_EQ(result, "123456789");

  // random string
  ledger::_environment = type::Environment::STAGING;
  result = uphold::GenerateRandomString(false);
  ASSERT_EQ(result.length(), 64u);
}

TEST_F(UpholdUtilTest, GenerateLinks) {
  ledger::_environment = type::Environment::STAGING;

  auto wallet = type::ExternalWallet::New();
  wallet->address = "123123123124234234234";

  // Not connected
  wallet->status = type::WalletStatus::NOT_CONNECTED;
  auto result = uphold::GenerateLinks(wallet->Clone());
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
  wallet->status = type::WalletStatus::CONNECTED;
  result = uphold::GenerateLinks(wallet->Clone());
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
  wallet->status = type::WalletStatus::VERIFIED;
  result = uphold::GenerateLinks(wallet->Clone());
  ASSERT_EQ(result->add_url,
      "https://sandbox.uphold.com/dashboard/cards/123123123124234234234/add");
  ASSERT_EQ(result->withdraw_url,
      "https://sandbox.uphold.com/dashboard/cards/123123123124234234234/use");
  ASSERT_EQ(result->verify_url, "");
  ASSERT_EQ(result->account_url, "https://sandbox.uphold.com/dashboard");

  // Disconnected Non-Verified
  wallet->status = type::WalletStatus::DISCONNECTED_NOT_VERIFIED;
  result = uphold::GenerateLinks(wallet->Clone());
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
  wallet->status = type::WalletStatus::DISCONNECTED_VERIFIED;
  result = uphold::GenerateLinks(wallet->Clone());
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
  wallet->status = type::WalletStatus::PENDING;
  result = uphold::GenerateLinks(wallet->Clone());
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

TEST_F(UpholdUtilTest, GenerateVerifyLink) {
  ledger::_environment = type::Environment::STAGING;

  auto wallet = type::ExternalWallet::New();
  wallet->one_time_string = "123123123124234234234";

  // Not connected
  wallet->status = type::WalletStatus::NOT_CONNECTED;
  auto result = uphold::GenerateVerifyLink(wallet->Clone());
  ASSERT_EQ(result,
      "https://sandbox.uphold.com/authorize/4c2b665ca060d912fec5c735c734859a06"
      "118cc8?scope=accounts:read "
      "accounts:write cards:read cards:write user:read "
      "transactions:deposit transactions:read "
      "transactions:transfer:application transactions:transfer:others"
      "&intention=kyc&state=123123123124234234234");

  // Connected
  wallet->status = type::WalletStatus::CONNECTED;
  result = uphold::GenerateVerifyLink(wallet->Clone());
  ASSERT_EQ(result,
      "https://sandbox.uphold.com/signup/step2?application_id="
      "4c2b665ca060d912fec5c735c734859a06118cc8&intention=kyc");

  // Verified
  wallet->status = type::WalletStatus::VERIFIED;
  result = uphold::GenerateVerifyLink(wallet->Clone());
  ASSERT_EQ(result, "");

  // Disconnected Non-Verified
  wallet->status = type::WalletStatus::DISCONNECTED_NOT_VERIFIED;
  result = uphold::GenerateVerifyLink(wallet->Clone());
  ASSERT_EQ(result,
      "https://sandbox.uphold.com/authorize/4c2b665ca060d912fec5c735c734859a06"
      "118cc8?scope=accounts:read "
      "accounts:write cards:read cards:write user:read "
      "transactions:deposit transactions:read "
      "transactions:transfer:application transactions:transfer:others"
      "&intention=kyc&state=123123123124234234234");

  // Disconnected Verified
  wallet->status = type::WalletStatus::DISCONNECTED_VERIFIED;
  result = uphold::GenerateVerifyLink(wallet->Clone());
  ASSERT_EQ(result,
      "https://sandbox.uphold.com/authorize/4c2b665ca060d912fec5c735c734859a06"
      "118cc8?scope=accounts:read "
      "accounts:write cards:read cards:write user:read "
      "transactions:deposit transactions:read "
      "transactions:transfer:application transactions:transfer:others"
      "&intention=kyc&state=123123123124234234234");

  // Pending
  wallet->status = type::WalletStatus::PENDING;
  result = uphold::GenerateVerifyLink(wallet->Clone());
  ASSERT_EQ(result,
          "https://sandbox.uphold.com/signup/step2?"
          "application_id=4c2b665ca060d912fec5c735c734859a06118cc8&"
          "intention=kyc");
}

}  // namespace uphold
}  // namespace ledger
