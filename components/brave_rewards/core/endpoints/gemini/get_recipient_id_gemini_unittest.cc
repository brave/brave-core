/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoints/gemini/get_recipient_id_gemini.h"

#include <string>
#include <tuple>
#include <utility>

#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/endpoints/request_for.h"
#include "brave/components/brave_rewards/core/test/rewards_engine_test.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::TestParamInfo;
using ::testing::Values;
using ::testing::WithParamInterface;

namespace brave_rewards::internal {

using Error = endpoints::GetRecipientIDGemini::Error;
using Result = endpoints::GetRecipientIDGemini::Result;

using GetRecipientIDGeminiParamType = std::tuple<
    std::string,          // test name suffix
    net::HttpStatusCode,  // GET recipient ID Gemini endpoint response status code
    std::string,          // GET recipient ID Gemini endpoint response body
    Result                // expected result
>;

class RewardsGetRecipientIDGeminiTest
    : public RewardsEngineTest,
      public WithParamInterface<GetRecipientIDGeminiParamType> {};

TEST_P(RewardsGetRecipientIDGeminiTest, Paths) {
  const auto& [ignore, status_code, body, expected_result] = GetParam();

  auto request_url = engine().Get<EnvironmentConfig>().gemini_api_url().Resolve(
      "/v1/payments/recipientIds");

  auto response = mojom::UrlResponse::New();
  response->status_code = status_code;
  response->body = body;

  client().AddNetworkResultForTesting(request_url.spec(), mojom::UrlMethod::GET,
                                      std::move(response));

  auto result = WaitFor<Result&&>([&](auto callback) {
    endpoints::RequestFor<endpoints::GetRecipientIDGemini>(engine(), "token")
        .Send(std::move(callback));
  });

  EXPECT_EQ(result, expected_result);
}

INSTANTIATE_TEST_SUITE_P(
    RewardsGetRecipientIDGeminiTest,
    RewardsGetRecipientIDGeminiTest,
    Values(
        GetRecipientIDGeminiParamType{"HTTP_200_success", net::HTTP_OK,
                                      R"(
        [
          {
            "label": "de476441-a834-4b93-82e3-3226e5153f73",
            "recipient_id": "621d392c-75b3-b655-94e4-2849a44d38a9"
          }, {
            "label": "Brave Browser",
            "recipient_id": "6378fc55-18db-488a-85a3-1af557767d0a"
          }
        ]
      )",
                                      "6378fc55-18db-488a-85a3-1af557767d0a"},
        GetRecipientIDGeminiParamType{
            "HTTP_200_no_recipient_id_with_brave_browser_label", net::HTTP_OK,
            R"(
        [
          {
            "label": "de476441-a834-4b93-82e3-3226e5153f73",
            "recipient_id": "621d392c-75b3-b655-94e4-2849a44d38a9"
          }, {
            "label": "not Brave Browser",
            "recipient_id": "6378fc55-18db-488a-85a3-1af557767d0a"
          }
        ]
      )",
            ""},
        GetRecipientIDGeminiParamType{
            "HTTP_200_failed_to_parse_body", net::HTTP_OK,
            R"(
        [
          {
            "label": "de476441-a834-4b93-82e3-3226e5153f73",
            "recipient_id": "621d392c-75b3-b655-94e4-2849a44d38a9"
          }, {
            "label": 42,
            "recipient_id": 42
          }
        ]
      )",
            base::unexpected(Error::kFailedToParseBody)},
        GetRecipientIDGeminiParamType{
            "HTTP_503_unexpected_status_code", net::HTTP_SERVICE_UNAVAILABLE,
            "", base::unexpected(Error::kUnexpectedStatusCode)}),
    [](const auto& info) { return std::get<0>(info.param); });

}  // namespace brave_rewards::internal
