/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <tuple>
#include <utility>

#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/endpoints/brave/patch_wallets.h"
#include "brave/components/brave_rewards/core/endpoints/request_for.h"
#include "brave/components/brave_rewards/core/rewards_engine_client_mock.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl_mock.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=*PatchWallets*

using ::testing::_;
using ::testing::TestParamInfo;
using ::testing::TestWithParam;
using ::testing::Values;

namespace brave_rewards::internal::endpoints::test {
using Error = PatchWallets::Error;
using Result = PatchWallets::Result;

// clang-format off
using PatchWalletsParamType = std::tuple<
    std::string,          // test name suffix
    net::HttpStatusCode,  // post create wallet endpoint response status code
    std::string,          // post create wallet endpoint response body
    Result                // expected result
>;
// clang-format on

class PatchWallets : public TestWithParam<PatchWalletsParamType> {
 protected:
  void SetUp() override {
    ON_CALL(*mock_engine_impl_.mock_client(),
            GetStringState(state::kWalletBrave, _))
        .WillByDefault([](const std::string&, auto callback) {
          std::string wallet = R"(
            {
              "payment_id": "fa5dea51-6af4-44ca-801b-07b6df3dcfe4",
              "recovery_seed": "AN6DLuI2iZzzDxpzywf+IKmK1nzFRarNswbaIDI3pQg="
            }
          )";
          std::move(callback).Run(std::move(wallet));
        });
  }

  base::test::TaskEnvironment task_environment_;
  MockRewardsEngineImpl mock_engine_impl_;
};

TEST_P(PatchWallets, Paths) {
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

  RequestFor<endpoints::PatchWallets>(mock_engine_impl_, "country_code")
      .Send(callback.Get());

  task_environment_.RunUntilIdle();
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
  Endpoints,
  PatchWallets,
  Values(
    PatchWalletsParamType{
      "0_HTTP_200_success",
      net::HTTP_OK,
      "",
      {}
    },
    PatchWalletsParamType{
      "1_HTTP_400_invalid_request",
      net::HTTP_BAD_REQUEST,
      "",
      base::unexpected(Error::kInvalidRequest)
    },
    PatchWalletsParamType{
      "2_HTTP_401_bad_request_signature",
      net::HTTP_UNAUTHORIZED,
      "",
      base::unexpected(Error::kBadRequestSignature)
    },
    PatchWalletsParamType{
      "3_HTTP_403_invalid_request",
      net::HTTP_FORBIDDEN,
      R"(
        {
          "message": "error updating rewards wallet: payment id does not match http signature key id",
          "code": 403
        }
      )",
      base::unexpected(Error::kInvalidRequest)
    },
    PatchWalletsParamType{
      "4_HTTP_403_request_signature_verification_failure",
      net::HTTP_FORBIDDEN,
      R"(
        {
          "message": "request signature verification failure",
          "code": 403
        }
      )",
      base::unexpected(Error::kRequestSignatureVerificationFailure)
    },
    PatchWalletsParamType{
      "5_HTTP_403_unknown_message",
      net::HTTP_FORBIDDEN,
      R"(
        {
          "message": "unknown message",
          "code": 403
        }
      )",
      base::unexpected(Error::kUnknownMessage)
    },
    PatchWalletsParamType{
      "6_HTTP_409_geo_country_already_declared",
      net::HTTP_CONFLICT,
      "",
      base::unexpected(Error::kGeoCountryAlreadyDeclared)
    },
    PatchWalletsParamType{
      "7_HTTP_500_unexpected_error",
      net::HTTP_INTERNAL_SERVER_ERROR,
      "",
      base::unexpected(Error::kUnexpectedError)
    },
    PatchWalletsParamType{
      "8_HTTP_503_unexpected_status_code",
      net::HTTP_SERVICE_UNAVAILABLE,
      "",
      base::unexpected(Error::kUnexpectedStatusCode)
    }),
  [](const TestParamInfo<PatchWalletsParamType>& info) {
    return std::get<0>(info.param);
  }
);
// clang-format on

}  // namespace brave_rewards::internal::endpoints::test
