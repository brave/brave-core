/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <tuple>
#include <utility>

#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "bat/ledger/internal/endpoints/bitflyer/post_commit_transaction/post_commit_transaction_bitflyer.h"
#include "bat/ledger/internal/endpoints/request_for.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=*PostCommitTransactionBitFlyer*

using ::testing::_;
using ::testing::Invoke;
using ::testing::TestWithParam;
using ::testing::Values;

namespace ledger::endpoints::test {
using Error = PostCommitTransactionBitFlyer::Error;
using Result = PostCommitTransactionBitFlyer::Result;

// clang-format off
using PostCommitTransactionBitFlyerParamType = std::tuple<
    std::string,          // test name suffix
    net::HttpStatusCode,  // post commit transaction endpoint status code
    std::string,          // post commit transaction endpoint response body
    Result                // expected result
>;
// clang-format on

class PostCommitTransactionBitFlyer
    : public TestWithParam<PostCommitTransactionBitFlyerParamType> {
 public:
  PostCommitTransactionBitFlyer(const PostCommitTransactionBitFlyer&) = delete;
  PostCommitTransactionBitFlyer& operator=(
      const PostCommitTransactionBitFlyer&) = delete;

  PostCommitTransactionBitFlyer(PostCommitTransactionBitFlyer&&) = delete;
  PostCommitTransactionBitFlyer& operator=(PostCommitTransactionBitFlyer&&) =
      delete;

 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  PostCommitTransactionBitFlyer()
      : mock_ledger_client_(), mock_ledger_impl_(&mock_ledger_client_) {}

  MockLedgerClient mock_ledger_client_;
  MockLedgerImpl mock_ledger_impl_;
};

TEST_P(PostCommitTransactionBitFlyer, Paths) {
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

  RequestFor<endpoints::PostCommitTransactionBitFlyer>(
      &mock_ledger_impl_, "token", "address",
      mojom::ExternalTransaction::New("transaction_id", "contribution_id",
                                      "destination", "amount"))
      .Send(base::BindLambdaForTesting(
          [expected_result = expected_result](Result&& result) {
            EXPECT_EQ(result, expected_result);
          }));
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
  Endpoints,
  PostCommitTransactionBitFlyer,
  Values(
    PostCommitTransactionBitFlyerParamType{
      "HTTP_200_success",
      net::HTTP_OK,
      "",
      {}
    },
    PostCommitTransactionBitFlyerParamType{
      "HTTP_401_access_token_expired",
      net::HTTP_UNAUTHORIZED,
      "",
      base::unexpected(Error::kAccessTokenExpired)
    },
    PostCommitTransactionBitFlyerParamType{
      "HTTP_409_response_not_a_dict",
      net::HTTP_CONFLICT,
      R"(
        [
          "amount": 0.95,
          "currency_code": "BAT",
          "dry_run": false,
          "message": null,
          "transfer_id": "transaction_id",
          "transfer_status": "SESSION_TIME_OUT"
        ]
      )",
      base::unexpected(Error::kFailedToParseBody)
    },
    PostCommitTransactionBitFlyerParamType{
      "HTTP_409_transfer_status_wrong_case",
      net::HTTP_CONFLICT,
      R"(
        {
          "amount": 0.95,
          "currency_code": "BAT",
          "dry_run": false,
          "message": null,
          "transfer_id": "transaction_id",
          "TRANSFER_STATUS": "SESSION_TIME_OUT"
        }
      )",
      base::unexpected(Error::kFailedToParseBody)
    },
    PostCommitTransactionBitFlyerParamType{
      "HTTP_409_access_token_expired",
      net::HTTP_CONFLICT,
      R"(
        {
          "amount": 0.95,
          "currency_code": "BAT",
          "dry_run": false,
          "message": null,
          "transfer_id": "transaction_id",
          "transfer_status": "SESSION_TIME_OUT"
        }
      )",
      base::unexpected(Error::kAccessTokenExpired)
    },
    PostCommitTransactionBitFlyerParamType{
      "HTTP_409_unexpected_error",
      net::HTTP_CONFLICT,
      R"(
        {
          "amount": 0.95,
          "currency_code": "BAT",
          "dry_run": false,
          "message": null,
          "transfer_id": "transaction_id",
          "transfer_status": "NOT_ALLOWED_TO_SEND"
        }
      )",
      base::unexpected(Error::kUnexpectedError)
    },
    PostCommitTransactionBitFlyerParamType{
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
