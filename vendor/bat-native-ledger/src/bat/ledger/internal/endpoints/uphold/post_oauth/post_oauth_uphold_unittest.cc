/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <tuple>
#include <utility>

#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "bat/ledger/internal/endpoints/request_for.h"
#include "bat/ledger/internal/endpoints/uphold/post_oauth/post_oauth_uphold.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=*PostOAuthUphold*

using ::testing::_;
using ::testing::Invoke;
using ::testing::TestParamInfo;
using ::testing::TestWithParam;
using ::testing::Values;

namespace ledger::endpoints::test {
using Error = PostOAuthUphold::Error;
using Result = PostOAuthUphold::Result;

// clang-format off
using PostOAuthUpholdParamType = std::tuple<
    std::string,          // test name suffix
    net::HttpStatusCode,  // post OAuth Uphold endpoint response status code
    std::string,          // post OAuth Uphold endpoint response body
    Result                // expected result
>;
// clang-format on

class PostOAuthUphold : public TestWithParam<PostOAuthUpholdParamType> {
 public:
  PostOAuthUphold(const PostOAuthUphold&) = delete;
  PostOAuthUphold& operator=(const PostOAuthUphold&) = delete;

  PostOAuthUphold(PostOAuthUphold&&) = delete;
  PostOAuthUphold& operator=(PostOAuthUphold&&) = delete;

 private:
  base::test::TaskEnvironment task_environment_;

 protected:
  PostOAuthUphold()
      : mock_ledger_client_(), mock_ledger_impl_(&mock_ledger_client_) {}

  MockLedgerClient mock_ledger_client_;
  MockLedgerImpl mock_ledger_impl_;
};

TEST_P(PostOAuthUphold, Paths) {
  const auto& [ignore, status_code, body, expected_result] = GetParam();

  ON_CALL(mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [status_code = status_code, body = body](
              mojom::UrlRequestPtr, client::LoadURLCallback callback) mutable {
            mojom::UrlResponse response;
            response.status_code = status_code;
            response.body = std::move(body);
            std::move(callback).Run(response);
          }));

  RequestFor<endpoints::PostOAuthUphold>(
      &mock_ledger_impl_, "bb50f9d4782fb86a4302ef18179033abb17c257f")
      .Send(base::BindLambdaForTesting(
          [expected_result = expected_result](Result&& result) {
            EXPECT_EQ(result, expected_result);
          }));
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
  Endpoints,
  PostOAuthUphold,
  Values(
    PostOAuthUpholdParamType{
      "HTTP_200_success",
      net::HTTP_OK,
      R"(
        {
          "access_token": "9fd84e43c803622cc65a63c6d380a47612d7d718",
          "expires_in": 7775999,
          "scope": "cards:read cards:write user:read transactions:transfer:application transactions:transfer:others",
          "token_type": "bearer"
        }
      )",
      "9fd84e43c803622cc65a63c6d380a47612d7d718"
    },
    PostOAuthUpholdParamType{
      "HTTP_200_failed_to_parse_body",
      net::HTTP_OK,
      R"(
        {
          "expires_in": 7775999,
          "scope": "cards:read cards:write user:read transactions:transfer:application transactions:transfer:others",
          "token_type": "bearer"
        }
      )",
      base::unexpected(Error::kFailedToParseBody)
    },
    PostOAuthUpholdParamType{
      "HTTP_503_unexpected_status_code",
      net::HTTP_SERVICE_UNAVAILABLE,
      "",
      base::unexpected(Error::kUnexpectedStatusCode)
    }),
  [](const TestParamInfo<PostOAuthUpholdParamType>& info) {
    return std::get<0>(info.param);
  }
);
// clang-format on

}  // namespace ledger::endpoints::test
