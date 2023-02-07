/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <tuple>
#include <utility>

#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "bat/ledger/internal/endpoints/request_for.h"
#include "bat/ledger/internal/endpoints/uphold/post_create_transaction/post_create_transaction_uphold.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=*PostCreateTransactionUphold*

using ::testing::_;
using ::testing::Invoke;
using ::testing::TestWithParam;
using ::testing::Values;

namespace ledger::endpoints::test {
using Error = PostCreateTransactionUphold::Error;
using Result = PostCreateTransactionUphold::Result;

// clang-format off
using PostCreateTransactionUpholdParamType = std::tuple<
    std::string,          // test name suffix
    net::HttpStatusCode,  // post create transaction endpoint status code
    std::string,          // post create transaction endpoint response body
    Result                // expected result
>;
// clang-format on

class PostCreateTransactionUphold
    : public TestWithParam<PostCreateTransactionUpholdParamType> {
 public:
  PostCreateTransactionUphold(const PostCreateTransactionUphold&) = delete;
  PostCreateTransactionUphold& operator=(const PostCreateTransactionUphold&) =
      delete;

  PostCreateTransactionUphold(PostCreateTransactionUphold&&) = delete;
  PostCreateTransactionUphold& operator=(PostCreateTransactionUphold&&) =
      delete;

 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  PostCreateTransactionUphold()
      : mock_ledger_client_(), mock_ledger_impl_(&mock_ledger_client_) {}

  MockLedgerClient mock_ledger_client_;
  MockLedgerImpl mock_ledger_impl_;
};

TEST_P(PostCreateTransactionUphold, Paths) {
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

  RequestFor<endpoints::PostCreateTransactionUphold>(
      &mock_ledger_impl_, "token", "address",
      mojom::ExternalTransaction::New("", "contribution_id", "destination",
                                      "amount"))
      .Send(base::BindLambdaForTesting(
          [expected_result = expected_result](Result&& result) {
            EXPECT_EQ(result, expected_result);
          }));
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
  Endpoints,
  PostCreateTransactionUphold,
  Values(
    PostCreateTransactionUpholdParamType{
      "HTTP_202_response_not_a_dict",
      net::HTTP_ACCEPTED,
      R"(
        [
          "id": "87725361-4245-4435-a75a-f7a85674714a"
        ]
      )",
      base::unexpected(Error::kFailedToParseBody)
    },
    PostCreateTransactionUpholdParamType{
      "HTTP_202_id_wrong_case",
      net::HTTP_ACCEPTED,
      R"(
        {
          "ID": "87725361-4245-4435-a75a-f7a85674714a"
        }
      )",
      base::unexpected(Error::kFailedToParseBody)
    },
    PostCreateTransactionUpholdParamType{
      "HTTP_202_ok",
      net::HTTP_ACCEPTED,
      R"(
        {
          "id": "87725361-4245-4435-a75a-f7a85674714a"
        }
      )",
      "87725361-4245-4435-a75a-f7a85674714a"
    },
    PostCreateTransactionUpholdParamType{
      "HTTP_401_access_token_expired",
      net::HTTP_UNAUTHORIZED,
      "",
      base::unexpected(Error::kAccessTokenExpired)
    },
    PostCreateTransactionUpholdParamType{
      "HTTP_500_unexpected_status_code",
      net::HTTP_INTERNAL_SERVER_ERROR,
      "",
      base::unexpected(Error::kUnexpectedStatusCode)
    }),
  [](const auto& info) {
    return std::get<0>(info.param);
  }
);
// clang-format on

}  // namespace ledger::endpoints::test
