/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>

#include "base/strings/strcat.h"
#include "base/test/task_environment.h"
#include "bat/ledger/buildflags.h"
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

  ~GeminiUtilTest() override = default;
};

TEST_F(GeminiUtilTest, GetClientId) {
  // production
  ledger::_environment = mojom::Environment::PRODUCTION;
  std::string result = gemini::GetClientId();
  ASSERT_EQ(result, BUILDFLAG(GEMINI_WALLET_CLIENT_ID));

  // staging
  ledger::_environment = mojom::Environment::STAGING;
  result = gemini::GetClientId();
  ASSERT_EQ(result, BUILDFLAG(GEMINI_WALLET_STAGING_CLIENT_ID));
}

TEST_F(GeminiUtilTest, GetClientSecret) {
  // production
  ledger::_environment = mojom::Environment::PRODUCTION;
  std::string result = gemini::GetClientSecret();
  ASSERT_EQ(result, BUILDFLAG(GEMINI_WALLET_CLIENT_SECRET));

  // staging
  ledger::_environment = mojom::Environment::STAGING;
  result = gemini::GetClientSecret();
  ASSERT_EQ(result, BUILDFLAG(GEMINI_WALLET_STAGING_CLIENT_SECRET));
}

TEST_F(GeminiUtilTest, GetFeeAddress) {
  // production
  ledger::_environment = mojom::Environment::PRODUCTION;
  std::string result = gemini::GetFeeAddress();
  ASSERT_EQ(result, kFeeAddressProduction);

  // staging
  ledger::_environment = mojom::Environment::STAGING;
  result = gemini::GetFeeAddress();
  ASSERT_EQ(result, kFeeAddressStaging);
}

TEST_F(GeminiUtilTest, GetLoginUrl) {
  // production
  ledger::_environment = mojom::Environment::PRODUCTION;
  std::string result = gemini::GetLoginUrl("my-state");
  ASSERT_EQ(result, base::StrCat({BUILDFLAG(GEMINI_OAUTH_URL),
                                  "/auth"
                                  "?client_id=",
                                  BUILDFLAG(GEMINI_WALLET_CLIENT_ID),
                                  "&scope="
                                  "balances:read,"
                                  "history:read,"
                                  "crypto:send,"
                                  "account:read,"
                                  "payments:create,"
                                  "payments:send,"
                                  "&redirect_uri=rewards://gemini/authorization"
                                  "&state=my-state"
                                  "&response_type=code"}));

  // staging
  ledger::_environment = mojom::Environment::STAGING;
  result = gemini::GetLoginUrl("my-state");
  ASSERT_EQ(result, base::StrCat({BUILDFLAG(GEMINI_OAUTH_STAGING_URL),
                                  "/auth"
                                  "?client_id=",
                                  BUILDFLAG(GEMINI_WALLET_STAGING_CLIENT_ID),
                                  "&scope="
                                  "balances:read,"
                                  "history:read,"
                                  "crypto:send,"
                                  "account:read,"
                                  "payments:create,"
                                  "payments:send,"
                                  "&redirect_uri=rewards://gemini/authorization"
                                  "&state=my-state"
                                  "&response_type=code"}));
}

TEST_F(GeminiUtilTest, GetActivityUrl) {
  // production
  ledger::_environment = mojom::Environment::PRODUCTION;
  std::string result = gemini::GetActivityUrl();
  ASSERT_EQ(result, base::StrCat({BUILDFLAG(GEMINI_OAUTH_URL), "/balances"}));

  // staging
  ledger::_environment = mojom::Environment::STAGING;
  result = gemini::GetActivityUrl();
  ASSERT_EQ(result,
            base::StrCat({BUILDFLAG(GEMINI_OAUTH_STAGING_URL), "/balances"}));
}

TEST_F(GeminiUtilTest, GetWallet) {
  // no wallet
  ON_CALL(*mock_ledger_client_, GetStringState(state::kWalletGemini))
      .WillByDefault(testing::Return(""));
  auto result = mock_ledger_impl_.get()->gemini()->GetWallet();
  ASSERT_TRUE(!result);

  const std::string wallet = FakeEncryption::Base64EncryptString(R"({
    "account_url": "https://exchange.sandbox.gemini.com",
    "address": "2323dff2ba-d0d1-4dfw-8e56-a2605bcaf4af",
    "fees": {},
    "login_url": "https://exchange.sandbox.gemini.com/auth",
    "one_time_string": "1F747AE0A708E47ED7E650BF1856B5A4EF7E36833BDB1158A108F8",
    "status": 2,
    "token": "4c80232r219c30cdf112208890a32c7e00",
    "user_name": "test"
  })");

  ON_CALL(*mock_ledger_client_, GetStringState(state::kWalletGemini))
      .WillByDefault(testing::Return(wallet));

  // Gemini wallet
  result = mock_ledger_impl_.get()->gemini()->GetWallet();
  ASSERT_TRUE(result);
  ASSERT_EQ(result->address, "2323dff2ba-d0d1-4dfw-8e56-a2605bcaf4af");
  ASSERT_EQ(result->user_name, "test");
  ASSERT_EQ(result->token, "4c80232r219c30cdf112208890a32c7e00");
  ASSERT_EQ(result->status, mojom::WalletStatus::kConnected);
}

TEST_F(GeminiUtilTest, GenerateRandomHexString) {
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

TEST_F(GeminiUtilTest, GenerateLinks) {
  ledger::_environment = mojom::Environment::STAGING;

  auto wallet = mojom::ExternalWallet::New();
  wallet->address = "123123123124234234234";
  wallet->one_time_string = "aaabbbccc";

  // Not connected
  wallet->status = mojom::WalletStatus::kNotConnected;
  auto result = gemini::GenerateLinks(wallet->Clone());
  ASSERT_EQ(result->login_url,
            base::StrCat(
                {BUILDFLAG(GEMINI_OAUTH_STAGING_URL),
                 "/auth?client_id=", BUILDFLAG(GEMINI_WALLET_STAGING_CLIENT_ID),
                 "&scope="
                 "balances:read,"
                 "history:read,"
                 "crypto:send,"
                 "account:read,"
                 "payments:create,"
                 "payments:send,"
                 "&redirect_uri=rewards://gemini/authorization"
                 "&state=aaabbbccc"
                 "&response_type=code"}));
  ASSERT_EQ(result->account_url, BUILDFLAG(GEMINI_OAUTH_STAGING_URL));

  // Connected
  wallet->status = mojom::WalletStatus::kConnected;
  result = gemini::GenerateLinks(wallet->Clone());
  ASSERT_EQ(result->login_url,
            base::StrCat(
                {BUILDFLAG(GEMINI_OAUTH_STAGING_URL),
                 "/auth?client_id=", BUILDFLAG(GEMINI_WALLET_STAGING_CLIENT_ID),
                 "&scope="
                 "balances:read,"
                 "history:read,"
                 "crypto:send,"
                 "account:read,"
                 "payments:create,"
                 "payments:send,"
                 "&redirect_uri=rewards://gemini/authorization"
                 "&state=aaabbbccc"
                 "&response_type=code"}));
  ASSERT_EQ(result->account_url, BUILDFLAG(GEMINI_OAUTH_STAGING_URL));

  // Logged out
  wallet->status = mojom::WalletStatus::kLoggedOut;
  result = gemini::GenerateLinks(wallet->Clone());
  ASSERT_EQ(result->login_url,
            base::StrCat(
                {BUILDFLAG(GEMINI_OAUTH_STAGING_URL),
                 "/auth?client_id=", BUILDFLAG(GEMINI_WALLET_STAGING_CLIENT_ID),
                 "&scope="
                 "balances:read,"
                 "history:read,"
                 "crypto:send,"
                 "account:read,"
                 "payments:create,"
                 "payments:send,"
                 "&redirect_uri=rewards://gemini/authorization"
                 "&state=aaabbbccc"
                 "&response_type=code"}));
  ASSERT_EQ(result->account_url, BUILDFLAG(GEMINI_OAUTH_STAGING_URL));
}

}  // namespace gemini
}  // namespace ledger
