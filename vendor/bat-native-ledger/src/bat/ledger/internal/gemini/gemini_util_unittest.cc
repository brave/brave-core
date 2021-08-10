/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>

#include "base/test/task_environment.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/common/random_util.h"
#include "bat/ledger/internal/gemini/gemini_util.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "bat/ledger/ledger.h"
#include "testing/gmock/include/gmock/gmock.h"

// npm run test -- brave_unit_tests --filter=GeminiUtilTest.*

using ::testing::_;

namespace ledger {
namespace gemini {

class GeminiUtilTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<ledger::MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<ledger::MockLedgerImpl> mock_ledger_impl_;

  GeminiUtilTest() {
    mock_ledger_client_ = std::make_unique<ledger::MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<ledger::MockLedgerImpl>(mock_ledger_client_.get());
  }

  ~GeminiUtilTest() override {}
};

TEST_F(GeminiUtilTest, GetClientId) {
  // production
  ledger::_environment = type::Environment::PRODUCTION;
  std::string result = gemini::GetClientId();
  ASSERT_EQ(result, GEMINI_WALLET_CLIENT_ID);

  // staging
  ledger::_environment = type::Environment::STAGING;
  result = gemini::GetClientId();
  ASSERT_EQ(result, GEMINI_WALLET_STAGING_CLIENT_ID);
}

TEST_F(GeminiUtilTest, GetClientSecret) {
  // production
  ledger::_environment = type::Environment::PRODUCTION;
  std::string result = gemini::GetClientSecret();
  ASSERT_EQ(result, GEMINI_WALLET_CLIENT_SECRET);

  // staging
  ledger::_environment = type::Environment::STAGING;
  result = gemini::GetClientSecret();
  ASSERT_EQ(result, GEMINI_WALLET_STAGING_CLIENT_SECRET);
}

TEST_F(GeminiUtilTest, GetFeeAddress) {
  // production
  ledger::_environment = type::Environment::PRODUCTION;
  std::string result = gemini::GetFeeAddress();
  ASSERT_EQ(result, kFeeAddressProduction);

  // staging
  ledger::_environment = type::Environment::STAGING;
  result = gemini::GetFeeAddress();
  ASSERT_EQ(result, kFeeAddressStaging);
}

TEST_F(GeminiUtilTest, GetAuthorizeUrl) {
  // production
  ledger::_environment = type::Environment::PRODUCTION;
  std::string result = gemini::GetAuthorizeUrl("my-state");
  ASSERT_EQ(result, GEMINI_OAUTH_URL
            "/auth"
            "?client_id=" GEMINI_WALLET_CLIENT_ID
            "&scope="
            "balances:read,"
            "history:read,"
            "crypto:send,"
            "account:read,"
            "payments:create,"
            "&redirect_uri=rewards://gemini/authorization"
            "&state=my-state"
            "&response_type=code");

  // staging
  ledger::_environment = type::Environment::STAGING;
  result = gemini::GetAuthorizeUrl("my-state");
  ASSERT_EQ(result, GEMINI_OAUTH_STAGING_URL
            "/auth"
            "?client_id=" GEMINI_WALLET_STAGING_CLIENT_ID
            "&scope="
            "balances:read,"
            "history:read,"
            "crypto:send,"
            "account:read,"
            "payments:create,"
            "&redirect_uri=rewards://gemini/authorization"
            "&state=my-state"
            "&response_type=code");
}

TEST_F(GeminiUtilTest, GetAddUrl) {
  // production
  ledger::_environment = type::Environment::PRODUCTION;
  std::string result = gemini::GetAddUrl();
  ASSERT_EQ(result, GEMINI_OAUTH_URL "/transfer/deposit");

  // staging
  ledger::_environment = type::Environment::STAGING;
  result = gemini::GetAddUrl();
  ASSERT_EQ(result, GEMINI_OAUTH_STAGING_URL "/transfer/deposit");
}

TEST_F(GeminiUtilTest, GetWithdrawUrl) {
  // production
  ledger::_environment = type::Environment::PRODUCTION;
  std::string result = gemini::GetWithdrawUrl();
  ASSERT_EQ(result, GEMINI_OAUTH_URL "/transfer/withdraw");

  // staging
  ledger::_environment = type::Environment::STAGING;
  result = gemini::GetWithdrawUrl();
  ASSERT_EQ(result, GEMINI_OAUTH_STAGING_URL "/transfer/withdraw");
}

TEST_F(GeminiUtilTest, GetWallet) {
  // no wallet
  ON_CALL(*mock_ledger_client_, GetStringState(state::kWalletGemini))
      .WillByDefault(testing::Return(""));
  auto result = mock_ledger_impl_.get()->gemini()->GetWallet();
  ASSERT_TRUE(!result);

  const std::string wallet = FakeEncryption::Base64EncryptString(R"({
    "account_url": "https://exchange.sandbox.gemini.com",
    "add_url": "",
    "address": "2323dff2ba-d0d1-4dfw-8e56-a2605bcaf4af",
    "fees": {},
    "login_url": "https://exchange.sandbox.gemini.com/auth",
    "one_time_string": "1F747AE0A708E47ED7E650BF1856B5A4EF7E36833BDB1158A108F8",
    "status": 2,
    "token": "4c80232r219c30cdf112208890a32c7e00",
    "user_name": "test",
    "verify_url": "https://exchange.sandbox.gemini.com/auth/token",
    "withdraw_url": ""
  })");

  ON_CALL(*mock_ledger_client_, GetStringState(state::kWalletGemini))
      .WillByDefault(testing::Return(wallet));

  // Gemini wallet
  result = mock_ledger_impl_.get()->gemini()->GetWallet();
  ASSERT_TRUE(result);
  ASSERT_EQ(result->address, "2323dff2ba-d0d1-4dfw-8e56-a2605bcaf4af");
  ASSERT_EQ(result->user_name, "test");
  ASSERT_EQ(result->token, "4c80232r219c30cdf112208890a32c7e00");
  ASSERT_EQ(result->status, type::WalletStatus::VERIFIED);
}

TEST_F(GeminiUtilTest, GenerateRandomHexString) {
  // string for testing
  ledger::is_testing = true;
  auto result = ledger::util::GenerateRandomHexString();
  ASSERT_EQ(result, "123456789");

  // random string
  ledger::is_testing = false;
  ledger::_environment = type::Environment::STAGING;
  result = ledger::util::GenerateRandomHexString();
  ASSERT_EQ(result.length(), 64u);
}

TEST_F(GeminiUtilTest, GenerateLinks) {
  ledger::_environment = type::Environment::STAGING;

  auto wallet = type::ExternalWallet::New();
  wallet->address = "123123123124234234234";
  wallet->one_time_string = "aaabbbccc";

  // Not connected
  wallet->status = type::WalletStatus::NOT_CONNECTED;
  auto result = gemini::GenerateLinks(wallet->Clone());
  ASSERT_EQ(result->add_url, "");
  ASSERT_EQ(result->withdraw_url, "");
  ASSERT_EQ(result->verify_url, GEMINI_OAUTH_STAGING_URL
            "/auth?client_id=" GEMINI_WALLET_STAGING_CLIENT_ID
            "&scope="
            "balances:read,"
            "history:read,"
            "crypto:send,"
            "account:read,"
            "payments:create,"
            "&redirect_uri=rewards://gemini/authorization"
            "&state=aaabbbccc"
            "&response_type=code");
  ASSERT_EQ(result->account_url, GEMINI_OAUTH_STAGING_URL);

  // Connected
  wallet->status = type::WalletStatus::CONNECTED;
  result = gemini::GenerateLinks(wallet->Clone());
  ASSERT_EQ(result->add_url, "");
  ASSERT_EQ(result->withdraw_url, "");
  ASSERT_EQ(result->verify_url, GEMINI_OAUTH_STAGING_URL
            "/auth?client_id=" GEMINI_WALLET_STAGING_CLIENT_ID
            "&scope="
            "balances:read,"
            "history:read,"
            "crypto:send,"
            "account:read,"
            "payments:create,"
            "&redirect_uri=rewards://gemini/authorization"
            "&state=aaabbbccc"
            "&response_type=code");
  ASSERT_EQ(result->account_url, GEMINI_OAUTH_STAGING_URL);

  // Verified
  wallet->status = type::WalletStatus::VERIFIED;
  result = gemini::GenerateLinks(wallet->Clone());
  ASSERT_EQ(result->add_url, GEMINI_OAUTH_STAGING_URL "/transfer/deposit");
  ASSERT_EQ(result->withdraw_url,
            GEMINI_OAUTH_STAGING_URL "/transfer/withdraw");
  ASSERT_EQ(result->verify_url, GEMINI_OAUTH_STAGING_URL
            "/auth?client_id=" GEMINI_WALLET_STAGING_CLIENT_ID
            "&scope="
            "balances:read,"
            "history:read,"
            "crypto:send,"
            "account:read,"
            "payments:create,"
            "&redirect_uri=rewards://gemini/authorization"
            "&state=aaabbbccc"
            "&response_type=code");
  ASSERT_EQ(result->account_url, GEMINI_OAUTH_STAGING_URL);

  // Disconnected Non-Verified
  wallet->status = type::WalletStatus::DISCONNECTED_NOT_VERIFIED;
  result = gemini::GenerateLinks(wallet->Clone());
  ASSERT_EQ(result->add_url, "");
  ASSERT_EQ(result->withdraw_url, "");
  ASSERT_EQ(result->verify_url, GEMINI_OAUTH_STAGING_URL
            "/auth?client_id=" GEMINI_WALLET_STAGING_CLIENT_ID
            "&scope="
            "balances:read,"
            "history:read,"
            "crypto:send,"
            "account:read,"
            "payments:create,"
            "&redirect_uri=rewards://gemini/authorization"
            "&state=aaabbbccc"
            "&response_type=code");
  ASSERT_EQ(result->account_url, GEMINI_OAUTH_STAGING_URL);

  // Disconnected Verified
  wallet->status = type::WalletStatus::DISCONNECTED_VERIFIED;
  result = gemini::GenerateLinks(wallet->Clone());
  ASSERT_EQ(result->add_url, "");
  ASSERT_EQ(result->withdraw_url, "");
  ASSERT_EQ(result->verify_url, GEMINI_OAUTH_STAGING_URL
            "/auth?client_id=" GEMINI_WALLET_STAGING_CLIENT_ID
            "&scope="
            "balances:read,"
            "history:read,"
            "crypto:send,"
            "account:read,"
            "payments:create,"
            "&redirect_uri=rewards://gemini/authorization"
            "&state=aaabbbccc"
            "&response_type=code");
  ASSERT_EQ(result->account_url, GEMINI_OAUTH_STAGING_URL);

  // Pending
  wallet->status = type::WalletStatus::PENDING;
  result = gemini::GenerateLinks(wallet->Clone());
  ASSERT_EQ(result->add_url, "");
  ASSERT_EQ(result->withdraw_url, "");
  ASSERT_EQ(result->verify_url, GEMINI_OAUTH_STAGING_URL
            "/auth?client_id=" GEMINI_WALLET_STAGING_CLIENT_ID
            "&scope="
            "balances:read,"
            "history:read,"
            "crypto:send,"
            "account:read,"
            "payments:create,"
            "&redirect_uri=rewards://gemini/authorization"
            "&state=aaabbbccc"
            "&response_type=code");
  ASSERT_EQ(result->account_url, GEMINI_OAUTH_STAGING_URL);
}

}  // namespace gemini
}  // namespace ledger
