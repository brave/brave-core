/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <tuple>
#include <utility>

#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "bat/ledger/internal/endpoints/gemini/post_commit_transaction/post_commit_transaction_gemini.h"
#include "bat/ledger/internal/endpoints/request_for.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=*PostCommitTransactionGemini*

using ::testing::_;
using ::testing::Invoke;
using ::testing::TestWithParam;
using ::testing::Values;

namespace ledger::endpoints::test {
using Error = PostCommitTransactionGemini::Error;
using Result = PostCommitTransactionGemini::Result;

// clang-format off
using PostCommitTransactionGeminiParamType = std::tuple<
    std::string,          // test name suffix
    net::HttpStatusCode,  // post commit transaction endpoint status code
    std::string,          // post commit transaction endpoint response body
    Result                // expected result
>;
// clang-format on

class PostCommitTransactionGemini
    : public TestWithParam<PostCommitTransactionGeminiParamType> {
 public:
  PostCommitTransactionGemini(const PostCommitTransactionGemini&) = delete;
  PostCommitTransactionGemini& operator=(const PostCommitTransactionGemini&) =
      delete;

  PostCommitTransactionGemini(PostCommitTransactionGemini&&) = delete;
  PostCommitTransactionGemini& operator=(PostCommitTransactionGemini&&) =
      delete;

 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  PostCommitTransactionGemini()
      : mock_ledger_client_(), mock_ledger_impl_(&mock_ledger_client_) {}

  MockLedgerClient mock_ledger_client_;
  MockLedgerImpl mock_ledger_impl_;
};

TEST_P(PostCommitTransactionGemini, Paths) {
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

  RequestFor<endpoints::PostCommitTransactionGemini>(
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
  PostCommitTransactionGemini,
  Values(
    PostCommitTransactionGeminiParamType{
      "HTTP_200_response_not_a_dict",
      net::HTTP_OK,
      R"(
        [
          "amount": 0.95,
          "currency": "BAT",
          "destination": "621e9ca3-6c64-4055-bce7-e3460841a7cc",
          "result": "OK",
          "status": "Pending",
          "tx_ref": "transaction_id"
        ]
      )",
      base::unexpected(Error::kFailedToParseBody)
    },
    PostCommitTransactionGeminiParamType{
      "HTTP_200_status_wrong_case",
      net::HTTP_OK,
      R"(
        {
          "amount": 0.95,
          "currency": "BAT",
          "destination": "621e9ca3-6c64-4055-bce7-e3460841a7cc",
          "result": "OK",
          "STATUS": "Pending",
          "tx_ref": "transaction_id"
        }
      )",
      base::unexpected(Error::kFailedToParseBody)
    },
    PostCommitTransactionGeminiParamType{
      "HTTP_200_transaction_pending",
      net::HTTP_OK,
      R"(
        {
          "amount": 0.95,
          "currency": "BAT",
          "destination": "621e9ca3-6c64-4055-bce7-e3460841a7cc",
          "result": "OK",
          "status": "Pending",
          "tx_ref": "transaction_id"
        }
      )",
      base::unexpected(Error::kTransactionPending)
    },
    PostCommitTransactionGeminiParamType{
      "HTTP_200_unknown_status",
      net::HTTP_OK,
      R"(
        {
          "amount": 0.95,
          "currency": "BAT",
          "destination": "621e9ca3-6c64-4055-bce7-e3460841a7cc",
          "result": "OK",
          "status": "UnknownStatus",
          "tx_ref": "transaction_id"
        }
      )",
      base::unexpected(Error::kUnexpectedError)
    },
    PostCommitTransactionGeminiParamType{
      "HTTP_200_transaction_completed",
      net::HTTP_OK,
      R"(
        {
          "amount": 0.95,
          "currency": "BAT",
          "destination": "621e9ca3-6c64-4055-bce7-e3460841a7cc",
          "result": "OK",
          "status": "Completed",
          "tx_ref": "transaction_id"
        }
      )",
      {}
    },
    PostCommitTransactionGeminiParamType{
      "HTTP_401_access_token_expired",
      net::HTTP_UNAUTHORIZED,
      "",
      base::unexpected(Error::kAccessTokenExpired)
    },
    PostCommitTransactionGeminiParamType{
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
