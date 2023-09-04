/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <tuple>
#include <utility>

#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/endpoints/gemini/get_recipient_id_gemini.h"
#include "brave/components/brave_rewards/core/endpoints/request_for.h"
#include "brave/components/brave_rewards/core/rewards_engine_client_mock.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl_mock.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=*GetRecipientIDGemini*

using ::testing::_;
using ::testing::TestParamInfo;
using ::testing::TestWithParam;
using ::testing::Values;

namespace brave_rewards::internal::endpoints::test {
using Error = GetRecipientIDGemini::Error;
using Result = GetRecipientIDGemini::Result;

// clang-format off
using GetRecipientIDGeminiParamType = std::tuple<
    std::string,          // test name suffix
    net::HttpStatusCode,  // GET recipient ID Gemini endpoint response status code
    std::string,          // GET recipient ID Gemini endpoint response body
    Result                // expected result
>;
// clang-format on

class GetRecipientIDGemini
    : public TestWithParam<GetRecipientIDGeminiParamType> {
 protected:
  base::test::TaskEnvironment task_environment_;
  MockRewardsEngineImpl mock_engine_impl_;
};

TEST_P(GetRecipientIDGemini, Paths) {
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

  RequestFor<endpoints::GetRecipientIDGemini>(mock_engine_impl_, "token")
      .Send(callback.Get());

  task_environment_.RunUntilIdle();
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
  Endpoints,
  GetRecipientIDGemini,
  Values(
    GetRecipientIDGeminiParamType{
      "HTTP_200_success",
      net::HTTP_OK,
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
      "6378fc55-18db-488a-85a3-1af557767d0a"
    },
    GetRecipientIDGeminiParamType{
      "HTTP_200_no_recipient_id_with_brave_browser_label",
      net::HTTP_OK,
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
      ""
    },
    GetRecipientIDGeminiParamType{
      "HTTP_200_failed_to_parse_body",
      net::HTTP_OK,
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
      base::unexpected(Error::kFailedToParseBody)
    },
    GetRecipientIDGeminiParamType{
      "HTTP_503_unexpected_status_code",
      net::HTTP_SERVICE_UNAVAILABLE,
      "",
      base::unexpected(Error::kUnexpectedStatusCode)
    }),
  [](const auto& info) {
    return std::get<0>(info.param);
  }
);
// clang-format on

}  // namespace brave_rewards::internal::endpoints::test
