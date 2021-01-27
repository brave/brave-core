/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <map>
#include <memory>
#include <utility>

#include "base/test/task_environment.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/bitflyer/bitflyer_util.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "bat/ledger/ledger.h"
#include "testing/gmock/include/gmock/gmock.h"

// npm run test -- brave_unit_tests --filter=BitflyerUtilTest.*

using ::testing::_;

namespace ledger {
namespace bitflyer {

class BitflyerUtilTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<ledger::MockLedgerImpl> mock_ledger_impl_;

  BitflyerUtilTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<ledger::MockLedgerImpl>(mock_ledger_client_.get());
  }

  ~BitflyerUtilTest() override {}
};

TEST_F(BitflyerUtilTest, GetClientId) {
  // production
  ledger::_environment = type::Environment::PRODUCTION;
  std::string result = bitflyer::GetClientId();
  ASSERT_EQ(result, kClientIdProduction);

  // staging
  ledger::_environment = type::Environment::STAGING;
  result = bitflyer::GetClientId();
  ASSERT_EQ(result, kClientIdStaging);
}

TEST_F(BitflyerUtilTest, GetFeeAddress) {
  // production
  ledger::_environment = type::Environment::PRODUCTION;
  std::string result = bitflyer::GetFeeAddress();
  ASSERT_EQ(result, kFeeAddressProduction);

  // staging
  ledger::_environment = type::Environment::STAGING;
  result = bitflyer::GetFeeAddress();
  ASSERT_EQ(result, kFeeAddressStaging);
}

TEST_F(BitflyerUtilTest, GetAuthorizeUrl) {
  // production
  ledger::_environment = type::Environment::PRODUCTION;
  std::string result = bitflyer::GetAuthorizeUrl("rdfdsfsdfsdf");
  ASSERT_EQ(result,
            "https://bitflyer.jp/ex/OAuth/authorize"
            "?client_id=078bf3871f3741432e813376d996a6a0"
            "&scope=assets create_deposit_id withdraw_to_deposit_id"
            "&redirect_uri=rewards://bitflyer/authorization"
            "&state=rdfdsfsdfsdf"
            "&response_type=code");

  // staging
  ledger::_environment = type::Environment::STAGING;
  result = bitflyer::GetAuthorizeUrl("rdfdsfsdfsdf");
  ASSERT_EQ(result, BITFLYER_STAGING_URL
            "/ex/OAuth/authorize"
            "?client_id=078bf3871f3741432e813376d996a6a0"
            "&scope=assets create_deposit_id withdraw_to_deposit_id"
            "&redirect_uri=rewards://bitflyer/authorization"
            "&state=rdfdsfsdfsdf"
            "&response_type=code");
}

TEST_F(BitflyerUtilTest, GetAddUrl) {
  // production
  ledger::_environment = type::Environment::PRODUCTION;
  std::string result = bitflyer::GetAddUrl();
  ASSERT_EQ(result, "https://bitflyer.jp/ex/Home?login=1");

  // staging
  ledger::_environment = type::Environment::STAGING;
  result = bitflyer::GetAddUrl();
  ASSERT_EQ(result, BITFLYER_STAGING_URL "/ex/Home?login=1");
}

TEST_F(BitflyerUtilTest, GetWithdrawUrl) {
  // production
  ledger::_environment = type::Environment::PRODUCTION;
  std::string result = bitflyer::GetWithdrawUrl();
  ASSERT_EQ(result, "https://bitflyer.jp/ex/Home?login=1");

  // staging
  ledger::_environment = type::Environment::STAGING;
  result = bitflyer::GetWithdrawUrl();
  ASSERT_EQ(result, BITFLYER_STAGING_URL "/ex/Home?login=1");
}

TEST_F(BitflyerUtilTest, GetWallet) {
  // no wallet
  ON_CALL(*mock_ledger_client_, GetEncryptedStringState(state::kWalletBitflyer))
      .WillByDefault(testing::Return(""));
  auto result = bitflyer::GetWallet(mock_ledger_impl_.get());
  ASSERT_TRUE(!result);

  const std::string wallet = R"({
    "account_url": "https://bitflyer.jp/ex/Home?login=1",
    "add_url": "",
    "address": "2323dff2ba-d0d1-4dfw-8e56-a2605bcaf4af",
    "fees": {},
    "login_url": "https://sandbox.bitflyer.jp/authorize/4c2b665ca060d",
    "one_time_string": "1F747AE0A708E47ED7E650BF1856B5A4EF7E36833BDB1158A108F8",
    "status": 2,
    "token": "4c80232r219c30cdf112208890a32c7e00",
    "user_name": "test",
    "verify_url": "https://sandbox.bitflyer.jp/authorize/4c2b665ca060d",
    "withdraw_url": ""
  })";

  ON_CALL(*mock_ledger_client_, GetEncryptedStringState(state::kWalletBitflyer))
      .WillByDefault(testing::Return(wallet));

  // Bitflyer wallet
  result = bitflyer::GetWallet(mock_ledger_impl_.get());
  ASSERT_TRUE(result);
  ASSERT_EQ(result->address, "2323dff2ba-d0d1-4dfw-8e56-a2605bcaf4af");
  ASSERT_EQ(result->user_name, "test");
  ASSERT_EQ(result->token, "4c80232r219c30cdf112208890a32c7e00");
  ASSERT_EQ(result->status, type::WalletStatus::VERIFIED);
}

TEST_F(BitflyerUtilTest, GenerateRandomString) {
  // string for testing
  auto result = bitflyer::GenerateRandomString(true);
  ASSERT_EQ(result, "123456789");

  // random string
  ledger::_environment = type::Environment::STAGING;
  result = bitflyer::GenerateRandomString(false);
  ASSERT_EQ(result.length(), 64u);
}

TEST_F(BitflyerUtilTest, GenerateLinks) {
  ledger::_environment = type::Environment::STAGING;

  auto wallet = type::ExternalWallet::New();
  wallet->address = "123123123124234234234";

  // Not connected
  wallet->status = type::WalletStatus::NOT_CONNECTED;
  auto result = bitflyer::GenerateLinks(wallet->Clone());
  ASSERT_EQ(result->add_url, "");
  ASSERT_EQ(result->withdraw_url, "");
  ASSERT_EQ(result->verify_url, BITFLYER_STAGING_URL
            "/ex/OAuth/authorize"
            "?client_id=078bf3871f3741432e813376d996a6a0"
            "&scope=assets create_deposit_id withdraw_to_deposit_id"
            "&redirect_uri=rewards://bitflyer/authorization"
            "&state=&response_type=code");
  ASSERT_EQ(result->account_url, BITFLYER_STAGING_URL "/ex/Home?login=1");

  // Connected
  wallet->status = type::WalletStatus::CONNECTED;
  result = bitflyer::GenerateLinks(wallet->Clone());
  ASSERT_EQ(result->add_url, "");
  ASSERT_EQ(result->withdraw_url, "");
  ASSERT_EQ(result->verify_url, BITFLYER_STAGING_URL
            "/ex/OAuth/authorize"
            "?client_id=078bf3871f3741432e813376d996a6a0"
            "&scope=assets create_deposit_id withdraw_to_deposit_id"
            "&redirect_uri=rewards://bitflyer/authorization"
            "&state=&response_type=code");
  ASSERT_EQ(result->account_url, BITFLYER_STAGING_URL "/ex/Home?login=1");

  // Verified
  wallet->status = type::WalletStatus::VERIFIED;
  result = bitflyer::GenerateLinks(wallet->Clone());
  ASSERT_EQ(result->add_url, BITFLYER_STAGING_URL "/ex/Home?login=1");
  ASSERT_EQ(result->withdraw_url, BITFLYER_STAGING_URL "/ex/Home?login=1");
  ASSERT_EQ(result->verify_url, BITFLYER_STAGING_URL
            "/ex/OAuth/authorize"
            "?client_id=078bf3871f3741432e813376d996a6a0"
            "&scope=assets create_deposit_id withdraw_to_deposit_id"
            "&redirect_uri=rewards://bitflyer/authorization"
            "&state=&response_type=code");
  ASSERT_EQ(result->account_url, BITFLYER_STAGING_URL "/ex/Home?login=1");

  // Disconnected Non-Verified
  wallet->status = type::WalletStatus::DISCONNECTED_NOT_VERIFIED;
  result = bitflyer::GenerateLinks(wallet->Clone());
  ASSERT_EQ(result->add_url, "");
  ASSERT_EQ(result->withdraw_url, "");
  ASSERT_EQ(result->verify_url, BITFLYER_STAGING_URL
            "/ex/OAuth/authorize"
            "?client_id=078bf3871f3741432e813376d996a6a0"
            "&scope=assets create_deposit_id withdraw_to_deposit_id"
            "&redirect_uri=rewards://bitflyer/authorization"
            "&state=&response_type=code");
  ASSERT_EQ(result->account_url, BITFLYER_STAGING_URL "/ex/Home?login=1");

  // Disconnected Verified
  wallet->status = type::WalletStatus::DISCONNECTED_VERIFIED;
  result = bitflyer::GenerateLinks(wallet->Clone());
  ASSERT_EQ(result->add_url, "");
  ASSERT_EQ(result->withdraw_url, "");
  ASSERT_EQ(result->verify_url, BITFLYER_STAGING_URL
            "/ex/OAuth/authorize"
            "?client_id=078bf3871f3741432e813376d996a6a0"
            "&scope=assets create_deposit_id withdraw_to_deposit_id"
            "&redirect_uri=rewards://bitflyer/authorization"
            "&state=&response_type=code");
  ASSERT_EQ(result->account_url, BITFLYER_STAGING_URL "/ex/Home?login=1");

  // Pending
  wallet->status = type::WalletStatus::PENDING;
  result = bitflyer::GenerateLinks(wallet->Clone());
  ASSERT_EQ(result->add_url, "");
  ASSERT_EQ(result->withdraw_url, "");
  ASSERT_EQ(result->verify_url, BITFLYER_STAGING_URL
            "/ex/OAuth/authorize"
            "?client_id=078bf3871f3741432e813376d996a6a0"
            "&scope=assets create_deposit_id withdraw_to_deposit_id"
            "&redirect_uri=rewards://bitflyer/authorization"
            "&state=&response_type=code");
  ASSERT_EQ(result->account_url, BITFLYER_STAGING_URL "/ex/Home?login=1");
}

}  // namespace bitflyer
}  // namespace ledger
