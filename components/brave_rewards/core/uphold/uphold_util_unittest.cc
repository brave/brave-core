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
#include "brave/components/brave_rewards/core/buildflags.h"
#include "brave/components/brave_rewards/core/common/random_util.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"
#include "brave/components/brave_rewards/core/rewards_engine_client_mock.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl_mock.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"
#include "brave/components/brave_rewards/core/test/test_rewards_engine_client.h"
#include "brave/components/brave_rewards/core/uphold/uphold.h"
#include "brave/components/brave_rewards/core/uphold/uphold_util.h"
#include "testing/gmock/include/gmock/gmock.h"

// npm run test -- brave_unit_tests --filter=*UpholdUtilTest.*

using ::testing::_;
using ::testing::Combine;
using ::testing::TestWithParam;
using ::testing::Values;

namespace brave_rewards::internal::uphold {

class UpholdUtilTest
    : public TestWithParam<
          std::tuple<mojom::Environment, mojom::WalletStatus>> {};

TEST_F(UpholdUtilTest, GetClientId) {
  _environment = mojom::Environment::PRODUCTION;
  EXPECT_EQ(GetClientId(), BUILDFLAG(UPHOLD_PRODUCTION_CLIENT_ID));

  _environment = mojom::Environment::STAGING;
  EXPECT_EQ(GetClientId(), BUILDFLAG(UPHOLD_SANDBOX_CLIENT_ID));

  _environment = mojom::Environment::DEVELOPMENT;
  EXPECT_EQ(GetClientId(), BUILDFLAG(UPHOLD_SANDBOX_CLIENT_ID));
}

TEST_F(UpholdUtilTest, GetClientSecret) {
  _environment = mojom::Environment::PRODUCTION;
  EXPECT_EQ(GetClientSecret(), BUILDFLAG(UPHOLD_PRODUCTION_CLIENT_SECRET));

  _environment = mojom::Environment::STAGING;
  EXPECT_EQ(GetClientSecret(), BUILDFLAG(UPHOLD_SANDBOX_CLIENT_SECRET));

  _environment = mojom::Environment::DEVELOPMENT;
  EXPECT_EQ(GetClientSecret(), BUILDFLAG(UPHOLD_SANDBOX_CLIENT_SECRET));
}

TEST_F(UpholdUtilTest, GetFeeAddress) {
  _environment = mojom::Environment::PRODUCTION;
  EXPECT_EQ(GetFeeAddress(), BUILDFLAG(UPHOLD_PRODUCTION_FEE_ADDRESS));

  _environment = mojom::Environment::STAGING;
  EXPECT_EQ(GetFeeAddress(), BUILDFLAG(UPHOLD_SANDBOX_FEE_ADDRESS));

  _environment = mojom::Environment::DEVELOPMENT;
  EXPECT_EQ(GetFeeAddress(), BUILDFLAG(UPHOLD_SANDBOX_FEE_ADDRESS));
}

TEST_F(UpholdUtilTest, GetServerUrl) {
  _environment = mojom::Environment::PRODUCTION;
  EXPECT_EQ(endpoint::uphold::GetServerUrl("/test"),
            base::StrCat({BUILDFLAG(UPHOLD_PRODUCTION_API_URL), "/test"}));

  _environment = mojom::Environment::STAGING;
  EXPECT_EQ(endpoint::uphold::GetServerUrl("/test"),
            base::StrCat({BUILDFLAG(UPHOLD_SANDBOX_API_URL), "/test"}));

  _environment = mojom::Environment::DEVELOPMENT;
  EXPECT_EQ(endpoint::uphold::GetServerUrl("/test"),
            base::StrCat({BUILDFLAG(UPHOLD_SANDBOX_API_URL), "/test"}));
}

TEST_F(UpholdUtilTest, GetWallet) {
  base::test::TaskEnvironment task_environment_;
  MockRewardsEngineImpl mock_engine_impl_;

  // no wallet
  ON_CALL(*mock_engine_impl_.mock_client(),
          GetStringState(state::kWalletUphold, _))
      .WillByDefault([](const std::string&, auto callback) {
        std::move(callback).Run("");
      });
  auto result = mock_engine_impl_.uphold()->GetWallet();
  EXPECT_FALSE(result);

  ON_CALL(*mock_engine_impl_.mock_client(),
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
  result = mock_engine_impl_.uphold()->GetWallet();
  EXPECT_TRUE(result);
  EXPECT_EQ(result->address, "2323dff2ba-d0d1-4dfw-8e56-a2605bcaf4af");
  EXPECT_EQ(result->user_name, "test");
  EXPECT_EQ(result->token, "4c80232r219c30cdf112208890a32c7e00");
  EXPECT_EQ(result->status, mojom::WalletStatus::kConnected);

  task_environment_.RunUntilIdle();
}

TEST_F(UpholdUtilTest, GenerateRandomHexString) {
  is_testing = true;
  auto result = util::GenerateRandomHexString();
  EXPECT_EQ(result, "123456789");

  is_testing = false;
  result = util::GenerateRandomHexString();
  EXPECT_EQ(result.length(), 64u);
}

INSTANTIATE_TEST_SUITE_P(GenerateLinks,
                         UpholdUtilTest,
                         Combine(Values(mojom::Environment::PRODUCTION,
                                        mojom::Environment::STAGING,
                                        mojom::Environment::DEVELOPMENT),
                                 Values(mojom::WalletStatus::kNotConnected,
                                        mojom::WalletStatus::kConnected,
                                        mojom::WalletStatus::kLoggedOut)),
                         [](const auto& info) {
                           return (std::ostringstream()
                                   << std::get<0>(info.param) << '_'
                                   << std::get<1>(info.param))
                               .str();
                         });

TEST_P(UpholdUtilTest, Paths) {
  const auto [environment, wallet_status] = GetParam();

  _environment = environment;
  auto wallet = mojom::ExternalWallet::New();
  wallet->status = wallet_status;
  wallet->address = "address";
  wallet->one_time_string = "one_time_string";

  const auto account_url =
      base::StrCat({environment == mojom::Environment::PRODUCTION
                        ? BUILDFLAG(UPHOLD_PRODUCTION_OAUTH_URL)
                        : BUILDFLAG(UPHOLD_SANDBOX_OAUTH_URL),
                    "/dashboard"});

  const auto activity_url =
      wallet->status == mojom::WalletStatus::kConnected
          ? base::StrCat({environment == mojom::Environment::PRODUCTION
                              ? BUILDFLAG(UPHOLD_PRODUCTION_OAUTH_URL)
                              : BUILDFLAG(UPHOLD_SANDBOX_OAUTH_URL),
                          "/dashboard/cards/address/activity"})
          : "";

  const auto login_url = base::StringPrintf(
      "%s/authorize/%s"
      "?scope="
      "cards:read "
      "cards:write "
      "user:read "
      "transactions:read "
      "transactions:transfer:application "
      "transactions:transfer:others"
      "&intention=login&"
      "state=one_time_string",
      environment == mojom::Environment::PRODUCTION
          ? BUILDFLAG(UPHOLD_PRODUCTION_OAUTH_URL)
          : BUILDFLAG(UPHOLD_SANDBOX_OAUTH_URL),
      environment == mojom::Environment::PRODUCTION
          ? BUILDFLAG(UPHOLD_PRODUCTION_CLIENT_ID)
          : BUILDFLAG(UPHOLD_SANDBOX_CLIENT_ID));

  EXPECT_FALSE(uphold::GenerateLinks(nullptr));
  const auto result = GenerateLinks(std::move(wallet));
  EXPECT_TRUE(result);
  EXPECT_EQ(result->account_url, account_url);
  EXPECT_EQ(result->activity_url, activity_url);
  EXPECT_EQ(result->login_url, login_url);
}

}  // namespace brave_rewards::internal::uphold
