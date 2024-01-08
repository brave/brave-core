/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <tuple>
#include <utility>

#include "base/strings/strcat.h"
#include "base/strings/stringprintf.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/bitflyer/bitflyer.h"
#include "brave/components/brave_rewards/core/bitflyer/bitflyer_util.h"
#include "brave/components/brave_rewards/core/buildflags.h"
#include "brave/components/brave_rewards/core/common/random_util.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"
#include "brave/components/brave_rewards/core/rewards_engine_client_mock.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl_mock.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"
#include "brave/components/brave_rewards/core/test/test_rewards_engine_client.h"
#include "testing/gmock/include/gmock/gmock.h"

// npm run test -- brave_unit_tests --filter=*BitflyerUtilTest.*

using ::testing::_;
using ::testing::Combine;
using ::testing::TestWithParam;
using ::testing::Values;

namespace brave_rewards::internal::bitflyer {

class BitflyerUtilTest
    : public TestWithParam<
          std::tuple<mojom::Environment, mojom::WalletStatus>> {};

TEST_F(BitflyerUtilTest, GetClientId) {
  _environment = mojom::Environment::PRODUCTION;
  EXPECT_EQ(GetClientId(), BUILDFLAG(BITFLYER_PRODUCTION_CLIENT_ID));

  _environment = mojom::Environment::STAGING;
  EXPECT_EQ(GetClientId(), BUILDFLAG(BITFLYER_SANDBOX_CLIENT_ID));

  _environment = mojom::Environment::DEVELOPMENT;
  EXPECT_EQ(GetClientId(), BUILDFLAG(BITFLYER_SANDBOX_CLIENT_ID));
}

TEST_F(BitflyerUtilTest, GetClientSecret) {
  _environment = mojom::Environment::PRODUCTION;
  EXPECT_EQ(GetClientSecret(), BUILDFLAG(BITFLYER_PRODUCTION_CLIENT_SECRET));

  _environment = mojom::Environment::STAGING;
  EXPECT_EQ(GetClientSecret(), BUILDFLAG(BITFLYER_SANDBOX_CLIENT_SECRET));

  _environment = mojom::Environment::DEVELOPMENT;
  EXPECT_EQ(GetClientSecret(), BUILDFLAG(BITFLYER_SANDBOX_CLIENT_SECRET));
}

TEST_F(BitflyerUtilTest, GetFeeAddress) {
  _environment = mojom::Environment::PRODUCTION;
  EXPECT_EQ(GetFeeAddress(), BUILDFLAG(BITFLYER_PRODUCTION_FEE_ADDRESS));

  _environment = mojom::Environment::STAGING;
  EXPECT_EQ(GetFeeAddress(), BUILDFLAG(BITFLYER_SANDBOX_FEE_ADDRESS));

  _environment = mojom::Environment::DEVELOPMENT;
  EXPECT_EQ(GetFeeAddress(), BUILDFLAG(BITFLYER_SANDBOX_FEE_ADDRESS));
}

TEST_F(BitflyerUtilTest, GetServerUrl) {
  _environment = mojom::Environment::PRODUCTION;
  EXPECT_EQ(endpoint::bitflyer::GetServerUrl("/test"),
            base::StrCat({BUILDFLAG(BITFLYER_PRODUCTION_URL), "/test"}));

  _environment = mojom::Environment::STAGING;
  EXPECT_EQ(endpoint::bitflyer::GetServerUrl("/test"),
            base::StrCat({BUILDFLAG(BITFLYER_SANDBOX_URL), "/test"}));

  _environment = mojom::Environment::DEVELOPMENT;
  EXPECT_EQ(endpoint::bitflyer::GetServerUrl("/test"),
            base::StrCat({BUILDFLAG(BITFLYER_SANDBOX_URL), "/test"}));
}

TEST_F(BitflyerUtilTest, GetWallet) {
  base::test::TaskEnvironment task_environment_;
  MockRewardsEngineImpl mock_engine_impl_;

  // no wallet
  EXPECT_CALL(*mock_engine_impl_.mock_client(),
              GetStringState(state::kWalletBitflyer, _))
      .Times(1)
      .WillOnce([](const std::string&, auto callback) {
        std::move(callback).Run("");
      });
  auto result = mock_engine_impl_.bitflyer()->GetWallet();
  EXPECT_FALSE(result);

  EXPECT_CALL(*mock_engine_impl_.mock_client(),
              GetStringState(state::kWalletBitflyer, _))
      .Times(1)
      .WillOnce([](const std::string&, auto callback) {
        std::string wallet = FakeEncryption::Base64EncryptString(R"({
          "account_url": "https://bitflyer.com/ex/Home?login=1",
          "address": "2323dff2ba-d0d1-4dfw-8e56-a2605bcaf4af",
          "fees": {},
          "status": 2,
          "token": "4c80232r219c30cdf112208890a32c7e00",
          "user_name": "test"
        })");
        std::move(callback).Run(std::move(wallet));
      });

  // Bitflyer wallet
  result = mock_engine_impl_.bitflyer()->GetWallet();
  EXPECT_TRUE(result);
  EXPECT_EQ(result->address, "2323dff2ba-d0d1-4dfw-8e56-a2605bcaf4af");
  EXPECT_EQ(result->user_name, "test");
  EXPECT_EQ(result->token, "4c80232r219c30cdf112208890a32c7e00");
  EXPECT_EQ(result->status, mojom::WalletStatus::kConnected);

  task_environment_.RunUntilIdle();
}

TEST_F(BitflyerUtilTest, GetLoginUrl) {
  const auto login_url = base::StringPrintf(
      "%s/ex/OAuth/authorize"
      "?client_id=%s"
      "&scope="
      "assets "
      "create_deposit_id "
      "withdraw_to_deposit_id"
      "&redirect_uri=rewards://bitflyer/authorization"
      "&state=one_time_string"
      "&response_type=code"
      "&code_challenge_method=S256"
      "&code_challenge=73oehA2tBul5grZPhXUGQwNAjxh69zNES8bu2bVD0EM",
      BUILDFLAG(BITFLYER_SANDBOX_URL), BUILDFLAG(BITFLYER_SANDBOX_CLIENT_ID));

  _environment = mojom::Environment::STAGING;
  EXPECT_EQ(GetLoginUrl("one_time_string", "code_verifier"), login_url);
}

TEST_F(BitflyerUtilTest, GetAccountUrl) {
  _environment = mojom::Environment::STAGING;
  EXPECT_EQ(GetAccountUrl(),
            std::string(BUILDFLAG(BITFLYER_SANDBOX_URL)) + "/ex/Home?login=1");
}

TEST_F(BitflyerUtilTest, GetActivityUrl) {
  _environment = mojom::Environment::STAGING;
  EXPECT_EQ(GetActivityUrl(), std::string(BUILDFLAG(BITFLYER_SANDBOX_URL)) +
                                  "/ja-jp/ex/tradehistory");
}

TEST_F(BitflyerUtilTest, GenerateRandomHexString) {
  is_testing = true;
  auto result = util::GenerateRandomHexString();
  EXPECT_EQ(result, "123456789");

  is_testing = false;
  result = util::GenerateRandomHexString();
  EXPECT_EQ(result.length(), 64u);
}

}  // namespace brave_rewards::internal::bitflyer
