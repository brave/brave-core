/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <tuple>
#include <utility>

#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/endpoints/request_for.h"
#include "brave/components/brave_rewards/core/endpoints/zebpay/post_oauth_zebpay.h"
#include "brave/components/brave_rewards/core/rewards_engine_client_mock.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl_mock.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=*PostOAuthZebPay*

using ::testing::_;
using ::testing::TestParamInfo;
using ::testing::TestWithParam;
using ::testing::Values;

namespace brave_rewards::internal::endpoints::test {
using Error = PostOAuthZebPay::Error;
using Result = PostOAuthZebPay::Result;

// clang-format off
using PostOAuthZebPayParamType = std::tuple<
    std::string,          // test name suffix
    net::HttpStatusCode,  // post OAuth ZebPay endpoint response status code
    std::string,          // post OAuth ZebPay endpoint response body
    Result                // expected result
>;
// clang-format on

class PostOAuthZebPay : public TestWithParam<PostOAuthZebPayParamType> {
 protected:
  base::test::TaskEnvironment task_environment_;
  MockRewardsEngineImpl mock_engine_impl_;
};

TEST_P(PostOAuthZebPay, Paths) {
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

  RequestFor<endpoints::PostOAuthZebPay>(mock_engine_impl_, "code")
      .Send(callback.Get());

  task_environment_.RunUntilIdle();
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
  Endpoints,
  PostOAuthZebPay,
  Values(
    PostOAuthZebPayParamType{
      "HTTP_200_success",
      net::HTTP_OK,
      R"(
        {
          "access_token": "af3f053dff93a12cc14c489d6bf13ed23698a4d91305e215cf097046ab72abbc",
          "linking_info": "xxx.eyJkZXBvc2l0SWQiOiIxMTExMSJ9.zzz"
        }
      )",
      std::tuple{
        "af3f053dff93a12cc14c489d6bf13ed23698a4d91305e215cf097046ab72abbc",
        "xxx.eyJkZXBvc2l0SWQiOiIxMTExMSJ9.zzz",
        "11111"
      }
    },
    PostOAuthZebPayParamType{
      "HTTP_200_body_is_not_a_dict",
      net::HTTP_OK,
      "[]",
      base::unexpected(Error::kFailedToParseBody)
    },
    PostOAuthZebPayParamType{
      "HTTP_200_access_token_is_empty",
      net::HTTP_OK,
      R"(
        {
          "access_token": ""
        }
      )",
      base::unexpected(Error::kFailedToParseBody)
    },
    PostOAuthZebPayParamType{
      "HTTP_200_linking_info_is_empty",
      net::HTTP_OK,
      R"(
        {
          "access_token": "af3f053dff93a12cc14c489d6bf13ed23698a4d91305e215cf097046ab72abbc",
          "linking_info": ""
        }
      )",
      base::unexpected(Error::kFailedToParseBody)
    },
    PostOAuthZebPayParamType{
      "HTTP_200_linking_info_not_enough_components",
      net::HTTP_OK,
      R"(
        {
          "access_token": "af3f053dff93a12cc14c489d6bf13ed23698a4d91305e215cf097046ab72abbc",
          "linking_info": "xxx.yyy"
        }
      )",
      base::unexpected(Error::kFailedToParseBody)
    },
    PostOAuthZebPayParamType{
      "HTTP_200_linking_info_payload_is_not_a_dict",
      net::HTTP_OK,
      R"(
        {
          "access_token": "af3f053dff93a12cc14c489d6bf13ed23698a4d91305e215cf097046ab72abbc",
          "linking_info": "xxx.W10=.zzz"
        }
      )",
      base::unexpected(Error::kFailedToParseBody)
    },
    PostOAuthZebPayParamType{
      "HTTP_200_deposit_id_is_empty",
      net::HTTP_OK,
      R"(
        {
          "access_token": "af3f053dff93a12cc14c489d6bf13ed23698a4d91305e215cf097046ab72abbc",
          "linking_info": "xxx.eyJkZXBvc2l0SWQiOiIifQ==.zzz"
        }
      )",
      base::unexpected(Error::kFailedToParseBody)
    },
    PostOAuthZebPayParamType{
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
