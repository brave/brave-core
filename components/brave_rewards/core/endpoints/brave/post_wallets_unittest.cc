/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoints/brave/post_wallets.h"

#include <string>
#include <tuple>
#include <utility>

#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/common/prefs.h"
#include "brave/components/brave_rewards/core/endpoints/request_for.h"
#include "brave/components/brave_rewards/core/test/rewards_engine_test.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::TestParamInfo;
using ::testing::Values;
using ::testing::WithParamInterface;

namespace brave_rewards::internal::endpoints {
using Error = PostWallets::Error;
using Result = PostWallets::Result;

using PostWalletsParamType = std::tuple<
    std::string,          // test name suffix
    net::HttpStatusCode,  // post create wallet endpoint response status code
    std::string,          // post create wallet endpoint response body
    Result                // expected result
>;

class RewardsPostWalletsTest : public RewardsEngineTest,
                               public WithParamInterface<PostWalletsParamType> {
 protected:
  void SetUp() override {
    engine().Get<Prefs>().SetString(prefs::kWalletBrave, R"(
        {
          "payment_id": "",
          "recovery_seed": "AN6DLuI2iZzzDxpzywf+IKmK1nzFRarNswbaIDI3pQg="
        })");
  }
};

TEST_P(RewardsPostWalletsTest, Paths) {
  const auto& [ignore, status_code, body, expected_result] = GetParam();

  auto request_url =
      engine().Get<EnvironmentConfig>().rewards_grant_url().Resolve(
          "/v4/wallets");

  auto response = mojom::UrlResponse::New();
  response->status_code = status_code;
  response->body = body;

  client().AddNetworkResultForTesting(
      request_url.spec(), mojom::UrlMethod::POST, std::move(response));

  auto result = WaitFor<Result&&>([&](auto callback) {
    RequestFor<endpoints::PostWallets>(engine(), "geo_country")
        .Send(std::move(callback));
  });

  EXPECT_EQ(result, expected_result);
}

INSTANTIATE_TEST_SUITE_P(
    RewardsPostWalletsTest,
    RewardsPostWalletsTest,
    Values(PostWalletsParamType{"0_HTTP_201_success", net::HTTP_CREATED,
                                R"(
        {
          "paymentId": "284a68ea-95ac-559a-b95c-5f07b4db0c72",
          "walletProvider": {
            "id": "",
            "name": "brave"
          },
          "altcurrency": "BAT",
          "publicKey": "7de76306129de620d01406cdd5a72c5e0ea2e427504f0faff2ba5788c81f2e76"
        }
      )",
                                "284a68ea-95ac-559a-b95c-5f07b4db0c72"},
           PostWalletsParamType{"1_HTTP_400_invalid_request",
                                net::HTTP_BAD_REQUEST, "",
                                base::unexpected(Error::kInvalidRequest)},
           PostWalletsParamType{"2_HTTP_401_invalid_public_key",
                                net::HTTP_UNAUTHORIZED, "",
                                base::unexpected(Error::kInvalidPublicKey)},
           PostWalletsParamType{
               "3_HTTP_403_wallet_generation_disabled", net::HTTP_FORBIDDEN, "",
               base::unexpected(Error::kWalletGenerationDisabled)},
           PostWalletsParamType{"4_HTTP_409_wallet_already_exists",
                                net::HTTP_CONFLICT, "",
                                base::unexpected(Error::kWalletAlreadyExists)},
           PostWalletsParamType{"5_HTTP_500_unexpected_error",
                                net::HTTP_INTERNAL_SERVER_ERROR, "",
                                base::unexpected(Error::kUnexpectedError)},
           PostWalletsParamType{
               "6_HTTP_503_unexpected_status_code",
               net::HTTP_SERVICE_UNAVAILABLE, "",
               base::unexpected(Error::kUnexpectedStatusCode)}),
    [](const TestParamInfo<PostWalletsParamType>& info) {
      return std::get<0>(info.param);
    });

}  // namespace brave_rewards::internal::endpoints
