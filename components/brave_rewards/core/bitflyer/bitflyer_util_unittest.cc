/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <map>
#include <memory>
#include <utility>

#include "base/strings/strcat.h"
#include "base/strings/stringprintf.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/bitflyer/bitflyer_util.h"
#include "brave/components/brave_rewards/core/buildflags.h"
#include "brave/components/brave_rewards/core/common/random_util.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/ledger.h"
#include "brave/components/brave_rewards/core/ledger_client_mock.h"
#include "brave/components/brave_rewards/core/ledger_impl_mock.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"
#include "testing/gmock/include/gmock/gmock.h"

// npm run test -- brave_unit_tests --filter=BitflyerUtilTest.*

using ::testing::_;

namespace brave_rewards::core {
namespace bitflyer {

class BitflyerUtilTest : public testing::Test {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<MockLedgerImpl> mock_ledger_impl_;

  BitflyerUtilTest() {
    mock_ledger_client_ = std::make_unique<MockLedgerClient>();
    mock_ledger_impl_ =
        std::make_unique<MockLedgerImpl>(mock_ledger_client_.get());
  }

  ~BitflyerUtilTest() override = default;
};

TEST_F(BitflyerUtilTest, GetClientId) {
  // production
  _environment = mojom::Environment::PRODUCTION;
  std::string result = bitflyer::GetClientId();
  ASSERT_EQ(result, BUILDFLAG(BITFLYER_CLIENT_ID));

  // staging
  _environment = mojom::Environment::STAGING;
  result = bitflyer::GetClientId();
  ASSERT_EQ(result, BUILDFLAG(BITFLYER_STAGING_CLIENT_ID));
}

TEST_F(BitflyerUtilTest, GetClientSecret) {
  // production
  _environment = mojom::Environment::PRODUCTION;
  std::string result = bitflyer::GetClientSecret();
  ASSERT_EQ(result, BUILDFLAG(BITFLYER_CLIENT_SECRET));

  // staging
  _environment = mojom::Environment::STAGING;
  result = bitflyer::GetClientSecret();
  ASSERT_EQ(result, BUILDFLAG(BITFLYER_STAGING_CLIENT_SECRET));
}

TEST_F(BitflyerUtilTest, GetFeeAddress) {
  // production
  _environment = mojom::Environment::PRODUCTION;
  std::string result = bitflyer::GetFeeAddress();
  ASSERT_EQ(result, kFeeAddressProduction);

  // staging
  _environment = mojom::Environment::STAGING;
  result = bitflyer::GetFeeAddress();
  ASSERT_EQ(result, kFeeAddressStaging);
}

TEST_F(BitflyerUtilTest, GetLoginUrl) {
  // production
  _environment = mojom::Environment::PRODUCTION;
  std::string result = bitflyer::GetLoginUrl("my-state", "my-code-verifier");
  ASSERT_EQ(
      result,
      base::StrCat(
          {kUrlProduction,
           "/ex/OAuth/authorize"
           "?client_id=",
           BUILDFLAG(BITFLYER_CLIENT_ID),
           "&scope=assets create_deposit_id withdraw_to_deposit_id"
           "&redirect_uri=rewards://bitflyer/authorization"
           "&state=my-state"
           "&response_type=code"
           "&code_challenge_method=S256"
           "&code_challenge=5Cxs3JXozcwTeteCIu4BcTieAhEIqjn643F10PxPD_w"}));

  // staging
  _environment = mojom::Environment::STAGING;
  result = bitflyer::GetLoginUrl("my-state", "my-code-verifier");
  ASSERT_EQ(
      result,
      base::StrCat(
          {BUILDFLAG(BITFLYER_STAGING_URL),
           "/ex/OAuth/authorize"
           "?client_id=",
           BUILDFLAG(BITFLYER_STAGING_CLIENT_ID),
           "&scope=assets create_deposit_id withdraw_to_deposit_id"
           "&redirect_uri=rewards://bitflyer/authorization"
           "&state=my-state"
           "&response_type=code"
           "&code_challenge_method=S256"
           "&code_challenge=5Cxs3JXozcwTeteCIu4BcTieAhEIqjn643F10PxPD_w"}));
}

TEST_F(BitflyerUtilTest, GetActivityUrl) {
  // production
  _environment = mojom::Environment::PRODUCTION;
  std::string result = bitflyer::GetActivityUrl();
  ASSERT_EQ(result, std::string(kUrlProduction) + "/ja-jp/ex/tradehistory");

  // staging
  _environment = mojom::Environment::STAGING;
  result = bitflyer::GetActivityUrl();
  ASSERT_EQ(result, base::StrCat({BUILDFLAG(BITFLYER_STAGING_URL),
                                  "/ja-jp/ex/tradehistory"}));
}

TEST_F(BitflyerUtilTest, GetWallet) {
  // no wallet
  ON_CALL(*mock_ledger_client_, GetStringState(state::kWalletBitflyer))
      .WillByDefault(testing::Return(""));
  auto result = mock_ledger_impl_.get()->bitflyer()->GetWallet();
  ASSERT_TRUE(!result);

  const std::string wallet = FakeEncryption::Base64EncryptString(R"({
    "account_url": "https://bitflyer.com/ex/Home?login=1",
    "address": "2323dff2ba-d0d1-4dfw-8e56-a2605bcaf4af",
    "fees": {},
    "login_url": "https://sandbox.bitflyer.com/authorize/4c2b665ca060d",
    "one_time_string": "1F747AE0A708E47ED7E650BF1856B5A4EF7E36833BDB1158A108F8",
    "code_verifier": "1234567890",
    "status": 2,
    "token": "4c80232r219c30cdf112208890a32c7e00",
    "user_name": "test"
  })");

  ON_CALL(*mock_ledger_client_, GetStringState(state::kWalletBitflyer))
      .WillByDefault(testing::Return(wallet));

  // Bitflyer wallet
  result = mock_ledger_impl_.get()->bitflyer()->GetWallet();
  ASSERT_TRUE(result);
  ASSERT_EQ(result->address, "2323dff2ba-d0d1-4dfw-8e56-a2605bcaf4af");
  ASSERT_EQ(result->user_name, "test");
  ASSERT_EQ(result->token, "4c80232r219c30cdf112208890a32c7e00");
  ASSERT_EQ(result->status, mojom::WalletStatus::kConnected);
}

TEST_F(BitflyerUtilTest, GenerateRandomHexString) {
  // string for testing
  is_testing = true;
  auto result = util::GenerateRandomHexString();
  ASSERT_EQ(result, "123456789");

  // random string
  is_testing = false;
  _environment = mojom::Environment::STAGING;
  result = util::GenerateRandomHexString();
  ASSERT_EQ(result.length(), 64u);
}

TEST_F(BitflyerUtilTest, GenerateLinks) {
  _environment = mojom::Environment::STAGING;

  auto wallet = mojom::ExternalWallet::New();
  wallet->address = "123123123124234234234";

  const char kLoginUrlTemplate[] =
      "%s/ex/OAuth/authorize"
      "?client_id=%s"
      "&scope=assets create_deposit_id withdraw_to_deposit_id"
      "&redirect_uri=rewards://bitflyer/authorization"
      "&state="
      "&response_type=code"
      "&code_challenge_method=S256"
      "&code_challenge=47DEQpj8HBSa-_TImW-5JCeuQeRkm5NMpJWZG3hSuFU";
  const auto kLoginUrl =
      base::StringPrintf(kLoginUrlTemplate, BUILDFLAG(BITFLYER_STAGING_URL),
                         BUILDFLAG(BITFLYER_STAGING_CLIENT_ID));
  const auto kStagingUrl =
      base::StrCat({BUILDFLAG(BITFLYER_STAGING_URL), "/ex/Home?login=1"});

  // Not connected
  wallet->status = mojom::WalletStatus::kNotConnected;
  auto result = bitflyer::GenerateLinks(wallet->Clone());
  ASSERT_EQ(result->login_url, kLoginUrl);
  ASSERT_EQ(result->account_url, kStagingUrl);

  // Connected
  wallet->status = mojom::WalletStatus::kConnected;
  result = bitflyer::GenerateLinks(wallet->Clone());
  ASSERT_EQ(result->login_url, kLoginUrl);
  ASSERT_EQ(result->account_url, kStagingUrl);

  // Logged out
  wallet->status = mojom::WalletStatus::kLoggedOut;
  result = bitflyer::GenerateLinks(wallet->Clone());
  ASSERT_EQ(result->login_url, kLoginUrl);
  ASSERT_EQ(result->account_url, kStagingUrl);
}

}  // namespace bitflyer
}  // namespace brave_rewards::core
