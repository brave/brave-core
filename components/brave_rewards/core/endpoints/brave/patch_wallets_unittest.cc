/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoints/brave/patch_wallets.h"

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
using Error = PatchWallets::Error;
using Result = PatchWallets::Result;

using PatchWalletsParamType = std::tuple<
    std::string,          // test name suffix
    net::HttpStatusCode,  // post create wallet endpoint response status code
    std::string,          // post create wallet endpoint response body
    Result                // expected result
>;

class RewardsPatchWalletsTest
    : public RewardsEngineTest,
      public WithParamInterface<PatchWalletsParamType> {
 protected:
  void SetUp() override {
    engine().Get<Prefs>().SetString(prefs::kWalletBrave, R"(
        {
          "payment_id": "fa5dea51-6af4-44ca-801b-07b6df3dcfe4",
          "recovery_seed": "AN6DLuI2iZzzDxpzywf+IKmK1nzFRarNswbaIDI3pQg="
        })");
  }
};

TEST_P(RewardsPatchWalletsTest, Paths) {
  const auto& [ignore, status_code, body, expected_result] = GetParam();

  auto request_url =
      engine().Get<EnvironmentConfig>().rewards_grant_url().Resolve(
          "/v4/wallets/fa5dea51-6af4-44ca-801b-07b6df3dcfe4");

  auto response = mojom::UrlResponse::New();
  response->status_code = status_code;
  response->body = body;

  client().AddNetworkResultForTesting(
      request_url.spec(), mojom::UrlMethod::PATCH, std::move(response));

  auto result = WaitFor<Result&&>([&](auto callback) {
    RequestFor<endpoints::PatchWallets>(engine(), "country_code")
        .Send(std::move(callback));
  });

  EXPECT_EQ(result, expected_result);
}

INSTANTIATE_TEST_SUITE_P(
    RewardsPatchWalletsTest,
    RewardsPatchWalletsTest,
    Values(
        PatchWalletsParamType{"0_HTTP_200_success", net::HTTP_OK, "", {}},
        PatchWalletsParamType{"1_HTTP_400_invalid_request",
                              net::HTTP_BAD_REQUEST, "",
                              base::unexpected(Error::kInvalidRequest)},
        PatchWalletsParamType{"2_HTTP_401_bad_request_signature",
                              net::HTTP_UNAUTHORIZED, "",
                              base::unexpected(Error::kBadRequestSignature)},
        PatchWalletsParamType{"3_HTTP_403_invalid_request", net::HTTP_FORBIDDEN,
                              R"(
        {
          "message": "error updating rewards wallet: payment id does not match http signature key id",
          "code": 403
        }
      )",
                              base::unexpected(Error::kInvalidRequest)},
        PatchWalletsParamType{
            "4_HTTP_403_request_signature_verification_failure",
            net::HTTP_FORBIDDEN,
            R"(
        {
          "message": "request signature verification failure",
          "code": 403
        }
      )",
            base::unexpected(Error::kRequestSignatureVerificationFailure)},
        PatchWalletsParamType{"5_HTTP_403_unknown_message", net::HTTP_FORBIDDEN,
                              R"(
        {
          "message": "unknown message",
          "code": 403
        }
      )",
                              base::unexpected(Error::kUnknownMessage)},
        PatchWalletsParamType{
            "6_HTTP_409_geo_country_already_declared", net::HTTP_CONFLICT, "",
            base::unexpected(Error::kGeoCountryAlreadyDeclared)},
        PatchWalletsParamType{"7_HTTP_500_unexpected_error",
                              net::HTTP_INTERNAL_SERVER_ERROR, "",
                              base::unexpected(Error::kUnexpectedError)},
        PatchWalletsParamType{"8_HTTP_503_unexpected_status_code",
                              net::HTTP_SERVICE_UNAVAILABLE, "",
                              base::unexpected(Error::kUnexpectedStatusCode)}),
    [](const TestParamInfo<PatchWalletsParamType>& info) {
      return std::get<0>(info.param);
    });

}  // namespace brave_rewards::internal::endpoints
