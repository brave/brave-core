/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoints/common/post_connect.h"

#include <string>
#include <tuple>
#include <utility>

#include "base/strings/stringprintf.h"
#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/endpoints/request_for.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"
#include "brave/components/brave_rewards/core/test/rewards_engine_test.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::TestParamInfo;
using ::testing::Values;
using ::testing::WithParamInterface;

namespace brave_rewards::internal {

using Error = endpoints::PostConnect::Error;
using Result = endpoints::PostConnect::Result;

class PostConnectMock final : public endpoints::PostConnect {
 public:
  explicit PostConnectMock(RewardsEngine& engine) : PostConnect(engine) {}
  ~PostConnectMock() override = default;

 private:
  std::string Path(base::cstring_view payment_id) const override {
    return base::StringPrintf("/v3/wallet/mock/%s/claim", payment_id.c_str());
  }
};

using PostConnectParamType = std::tuple<
    std::string,          // test name suffix
    net::HttpStatusCode,  // connect endpoint response status code
    std::string,          // connect endpoint response body
    Result                // expected result
>;

class RewardsPostConnectTest : public RewardsEngineTest,
                               public WithParamInterface<PostConnectParamType> {
 protected:
  void SetUp() override {
    engine().SetState<std::string>(state::kWalletBrave, R"(
        {
          "payment_id": "fa5dea51-6af4-44ca-801b-07b6df3dcfe4",
          "recovery_seed": "AN6DLuI2iZzzDxpzywf+IKmK1nzFRarNswbaIDI3pQg="
        })");
  }
};

TEST_P(RewardsPostConnectTest, Paths) {
  const auto& [ignore, status_code, body, expected_result] = GetParam();

  auto request_url =
      engine().Get<EnvironmentConfig>().rewards_grant_url().Resolve(
          "/v3/wallet/mock/fa5dea51-6af4-44ca-801b-07b6df3dcfe4/claim");

  auto response = mojom::UrlResponse::New();
  response->status_code = status_code;
  response->body = body;

  client().AddNetworkResultForTesting(
      request_url.spec(), mojom::UrlMethod::POST, std::move(response));

  auto result = WaitFor<Result&&>([&](auto callback) {
    endpoints::RequestFor<PostConnectMock>(engine()).Send(std::move(callback));
  });

  EXPECT_EQ(result, expected_result);
}

INSTANTIATE_TEST_SUITE_P(
    RewardsPostConnectTest,
    RewardsPostConnectTest,
    Values(
        PostConnectParamType{"HTTP_200_success", net::HTTP_OK,
                             R"(
        {
          "geoCountry": "US"
        }
      )",
                             "US"},
        PostConnectParamType{"HTTP_200_no_geo", net::HTTP_OK, "{}",
                             base::unexpected(Error::kFailedToParseBody)},
        PostConnectParamType{"HTTP_200_empty_geo", net::HTTP_OK,
                             R"(
        {
          "geoCountry": ""
        }
      )",
                             base::unexpected(Error::kFailedToParseBody)},
        PostConnectParamType{"HTTP_400_flagged_wallet", net::HTTP_BAD_REQUEST,
                             R"(
        {
          "message": "unable to link - unusual activity",
          "code": 400
        }
      )",
                             base::unexpected(Error::kFlaggedWallet)},
        PostConnectParamType{"HTTP_400_mismatched_countries",
                             net::HTTP_BAD_REQUEST,
                             R"(
        {
          "message": "error linking wallet: mismatched provider account regions: geo reset is different",
          "code": 400
        }
      )",
                             base::unexpected(Error::kMismatchedCountries)},
        PostConnectParamType{"HTTP_400_provider_unavailable",
                             net::HTTP_BAD_REQUEST,
                             R"(
        {
          "message": "Error validating Connecting Brave Rewards to Uphold is temporarily unavailable. Please try again later",
          "code": 400
        }
      )",
                             base::unexpected(Error::kProviderUnavailable)},
        PostConnectParamType{"HTTP_400_region_not_supported",
                             net::HTTP_BAD_REQUEST,
                             R"(
        {
          "message": "region not supported: failed to validate account: invalid country",
          "code": 400
        }
      )",
                             base::unexpected(Error::kRegionNotSupported)},
        PostConnectParamType{"HTTP_400_unknown_message", net::HTTP_BAD_REQUEST,
                             R"(
        {
          "message": "unknown message",
          "code": 400
        }
      )",
                             base::unexpected(Error::kUnknownMessage)},
        PostConnectParamType{"HTTP_403_kyc_required", net::HTTP_FORBIDDEN,
                             R"(
        {
          "message": "error linking wallet: KYC required: user kyc did not pass",
          "code": 403
        }
      )",
                             base::unexpected(Error::kKYCRequired)},
        PostConnectParamType{
            "HTTP_403_mismatched_provider_accounts", net::HTTP_FORBIDDEN,
            R"(
        {
          "message": "error linking wallet: unable to link wallets: mismatched provider accounts: wallets do not match",
          "code": 403
        }
      )",
            base::unexpected(Error::kMismatchedProviderAccounts)},
        PostConnectParamType{
            "HTTP_403_request_signature_verification_failure",
            net::HTTP_FORBIDDEN,
            R"(
        {
          "message": "request signature verification failure",
          "code": 403
        }
      )",
            base::unexpected(Error::kRequestSignatureVerificationFailure)},
        PostConnectParamType{
            "HTTP_403_transaction_verification_failure", net::HTTP_FORBIDDEN,
            R"(
        {
          "message": "error linking wallet: transaction verification failure: failed to verify transaction",
          "code": 403
        }
      )",
            base::unexpected(Error::kTransactionVerificationFailure)},
        PostConnectParamType{"HTTP_403_unknown_message", net::HTTP_FORBIDDEN,
                             R"(
        {
          "message": "unknown message",
          "code": 403
        }
      )",
                             base::unexpected(Error::kUnknownMessage)},
        PostConnectParamType{"HTTP_404_kyc_required", net::HTTP_NOT_FOUND, "",
                             base::unexpected(Error::kKYCRequired)},
        PostConnectParamType{"HTTP_409_device_limit_reached",
                             net::HTTP_CONFLICT, "",
                             base::unexpected(Error::kDeviceLimitReached)},
        PostConnectParamType{"HTTP_500_unexpected_error",
                             net::HTTP_INTERNAL_SERVER_ERROR, "",
                             base::unexpected(Error::kUnexpectedError)},
        PostConnectParamType{"HTTP_503_unexpected_status_code",
                             net::HTTP_SERVICE_UNAVAILABLE, "",
                             base::unexpected(Error::kUnexpectedStatusCode)}),
    [](const TestParamInfo<PostConnectParamType>& info) {
      return std::get<0>(info.param);
    });

}  // namespace brave_rewards::internal
