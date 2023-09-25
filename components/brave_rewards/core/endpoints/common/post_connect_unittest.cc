/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <tuple>
#include <utility>

#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/endpoints/common/post_connect.h"
#include "brave/components/brave_rewards/core/endpoints/request_for.h"
#include "brave/components/brave_rewards/core/rewards_engine_client_mock.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl_mock.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=*PostConnect*

using ::testing::_;
using ::testing::TestParamInfo;
using ::testing::TestWithParam;
using ::testing::Values;

namespace brave_rewards::internal::endpoints::test {
using Error = PostConnect::Error;
using Result = PostConnect::Result;

class PostConnectMock final : public PostConnect {
 public:
  explicit PostConnectMock(RewardsEngineImpl& engine) : PostConnect(engine) {}
  ~PostConnectMock() override = default;

 private:
  const char* Path() const override { return "/v3/wallet/mock/%s/claim"; }
};

// clang-format off
using PostConnectParamType = std::tuple<
    std::string,          // test name suffix
    net::HttpStatusCode,  // connect endpoint response status code
    std::string,          // connect endpoint response body
    Result                // expected result
>;
// clang-format on

class PostConnect : public TestWithParam<PostConnectParamType> {
 protected:
  void SetUp() override {
    ON_CALL(*mock_engine_impl_.mock_client(),
            GetStringState(state::kWalletBrave, _))
        .WillByDefault([](const std::string&, auto callback) {
          std::move(callback).Run(R"(
            {
              "payment_id":"fa5dea51-6af4-44ca-801b-07b6df3dcfe4",
              "recovery_seed":"AN6DLuI2iZzzDxpzywf+IKmK1nzFRarNswbaIDI3pQg="
            }
          )");
        });
  }

  base::test::TaskEnvironment task_environment_;
  MockRewardsEngineImpl mock_engine_impl_;
};

TEST_P(PostConnect, Paths) {
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

  RequestFor<PostConnectMock>(mock_engine_impl_).Send(callback.Get());

  task_environment_.RunUntilIdle();
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
  Endpoints,
  PostConnect,
  Values(
    PostConnectParamType{
      "HTTP_200_success",
      net::HTTP_OK,
      R"(
        {
          "geoCountry": "US"
        }
      )",
      "US"
    },
    PostConnectParamType{
      "HTTP_200_no_geo",
      net::HTTP_OK,
      "{}",
      base::unexpected(Error::kFailedToParseBody)
    },
    PostConnectParamType{
      "HTTP_200_empty_geo",
      net::HTTP_OK,
      R"(
        {
          "geoCountry": ""
        }
      )",
      base::unexpected(Error::kFailedToParseBody)
    },
    PostConnectParamType{
      "HTTP_400_flagged_wallet",
      net::HTTP_BAD_REQUEST,
      R"(
        {
          "message": "unable to link - unusual activity",
          "code": 400
        }
      )",
      base::unexpected(Error::kFlaggedWallet)
    },
    PostConnectParamType{
      "HTTP_400_mismatched_countries",
      net::HTTP_BAD_REQUEST,
      R"(
        {
          "message": "error linking wallet: mismatched provider account regions: geo reset is different",
          "code": 400
        }
      )",
      base::unexpected(Error::kMismatchedCountries)
    },
    PostConnectParamType{
      "HTTP_400_provider_unavailable",
      net::HTTP_BAD_REQUEST,
      R"(
        {
          "message": "Error validating Connecting Brave Rewards to Uphold is temporarily unavailable. Please try again later",
          "code": 400
        }
      )",
      base::unexpected(Error::kProviderUnavailable)
    },
    PostConnectParamType{
      "HTTP_400_region_not_supported",
      net::HTTP_BAD_REQUEST,
      R"(
        {
          "message": "region not supported: failed to validate account: invalid country",
          "code": 400
        }
      )",
      base::unexpected(Error::kRegionNotSupported)
    },
    PostConnectParamType{
      "HTTP_400_unknown_message",
      net::HTTP_BAD_REQUEST,
      R"(
        {
          "message": "unknown message",
          "code": 400
        }
      )",
      base::unexpected(Error::kUnknownMessage)
    },
    PostConnectParamType{
      "HTTP_403_kyc_required",
      net::HTTP_FORBIDDEN,
      R"(
        {
          "message": "error linking wallet: KYC required: user kyc did not pass",
          "code": 403
        }
      )",
      base::unexpected(Error::kKYCRequired)
    },
    PostConnectParamType{
      "HTTP_403_mismatched_provider_accounts",
      net::HTTP_FORBIDDEN,
      R"(
        {
          "message": "error linking wallet: unable to link wallets: mismatched provider accounts: wallets do not match",
          "code": 403
        }
      )",
      base::unexpected(Error::kMismatchedProviderAccounts)
    },
    PostConnectParamType{
      "HTTP_403_request_signature_verification_failure",
      net::HTTP_FORBIDDEN,
      R"(
        {
          "message": "request signature verification failure",
          "code": 403
        }
      )",
      base::unexpected(Error::kRequestSignatureVerificationFailure)
    },
    PostConnectParamType{
      "HTTP_403_transaction_verification_failure",
      net::HTTP_FORBIDDEN,
      R"(
        {
          "message": "error linking wallet: transaction verification failure: failed to verify transaction",
          "code": 403
        }
      )",
      base::unexpected(Error::kTransactionVerificationFailure)
    },
    PostConnectParamType{
      "HTTP_403_unknown_message",
      net::HTTP_FORBIDDEN,
      R"(
        {
          "message": "unknown message",
          "code": 403
        }
      )",
      base::unexpected(Error::kUnknownMessage)
    },
    PostConnectParamType{
      "HTTP_404_kyc_required",
      net::HTTP_NOT_FOUND,
      "",
      base::unexpected(Error::kKYCRequired)
    },
    PostConnectParamType{
      "HTTP_409_device_limit_reached",
      net::HTTP_CONFLICT,
      "",
      base::unexpected(Error::kDeviceLimitReached)
    },
    PostConnectParamType{
      "HTTP_500_unexpected_error",
      net::HTTP_INTERNAL_SERVER_ERROR,
      "",
      base::unexpected(Error::kUnexpectedError)
    },
    PostConnectParamType{
      "HTTP_503_unexpected_status_code",
      net::HTTP_SERVICE_UNAVAILABLE,
      "",
      base::unexpected(Error::kUnexpectedStatusCode)
    }),
  [](const TestParamInfo<PostConnectParamType>& info) {
    return std::get<0>(info.param);
  }
);
// clang-format on

}  // namespace brave_rewards::internal::endpoints::test
