/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <tuple>
#include <utility>

#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/endpoints/brave/post_wallets.h"
#include "brave/components/brave_rewards/core/endpoints/request_for.h"
#include "brave/components/brave_rewards/core/rewards_engine_client_mock.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl_mock.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=*PostWallets*

using ::testing::_;
using ::testing::TestParamInfo;
using ::testing::TestWithParam;
using ::testing::Values;

namespace brave_rewards::internal::endpoints::test {
using Error = PostWallets::Error;
using Result = PostWallets::Result;

// clang-format off
using PostWalletsParamType = std::tuple<
    std::string,          // test name suffix
    net::HttpStatusCode,  // post create wallet endpoint response status code
    std::string,          // post create wallet endpoint response body
    Result                // expected result
>;
// clang-format on

class PostWallets : public TestWithParam<PostWalletsParamType> {
 protected:
  void SetUp() override {
    ON_CALL(*mock_engine_impl_.mock_client(),
            GetStringState(state::kWalletBrave, _))
        .WillByDefault([](const std::string&, auto callback) {
          std::string wallet = R"(
            {
              "payment_id": "",
              "recovery_seed": "AN6DLuI2iZzzDxpzywf+IKmK1nzFRarNswbaIDI3pQg="
            }
          )";
          std::move(callback).Run(std::move(wallet));
        });
  }

  base::test::TaskEnvironment task_environment_;
  MockRewardsEngineImpl mock_engine_impl_;
};

TEST_P(PostWallets, Paths) {
  const auto& [ignore, status_code, body, expected_result] = GetParam();

  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([&](mojom::UrlRequestPtr, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = status_code;
        response->body = body;
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<base::OnceCallback<void(Result&&)>> callback;
  EXPECT_CALL(callback, Run(Result(expected_result))).Times(1);

  RequestFor<endpoints::PostWallets>(mock_engine_impl_, "geo_country")
      .Send(callback.Get());

  task_environment_.RunUntilIdle();
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
  Endpoints,
  PostWallets,
  Values(
    PostWalletsParamType{
      "0_HTTP_201_success",
      net::HTTP_CREATED,
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
      "284a68ea-95ac-559a-b95c-5f07b4db0c72"
    },
    PostWalletsParamType{
      "1_HTTP_400_invalid_request",
      net::HTTP_BAD_REQUEST,
      "",
      base::unexpected(Error::kInvalidRequest)
    },
    PostWalletsParamType{
      "2_HTTP_401_invalid_public_key",
      net::HTTP_UNAUTHORIZED,
      "",
      base::unexpected(Error::kInvalidPublicKey)
    },
    PostWalletsParamType{
      "3_HTTP_403_wallet_generation_disabled",
      net::HTTP_FORBIDDEN,
      "",
      base::unexpected(Error::kWalletGenerationDisabled)
    },
    PostWalletsParamType{
      "4_HTTP_409_wallet_already_exists",
      net::HTTP_CONFLICT,
      "",
      base::unexpected(Error::kWalletAlreadyExists)
    },
    PostWalletsParamType{
      "5_HTTP_500_unexpected_error",
      net::HTTP_INTERNAL_SERVER_ERROR,
      "",
      base::unexpected(Error::kUnexpectedError)
    },
    PostWalletsParamType{
      "6_HTTP_503_unexpected_status_code",
      net::HTTP_SERVICE_UNAVAILABLE,
      "",
      base::unexpected(Error::kUnexpectedStatusCode)
    }),
  [](const TestParamInfo<PostWalletsParamType>& info) {
    return std::get<0>(info.param);
  }
);
// clang-format on

}  // namespace brave_rewards::internal::endpoints::test
