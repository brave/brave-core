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
#include "bat/ledger/internal/endpoints/uphold/get_transaction_status/get_transaction_status_uphold.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=*GetTransactionStatusUphold*

using ::testing::_;
using ::testing::Invoke;
using ::testing::TestWithParam;
using ::testing::Values;

namespace ledger::endpoints::test {
using Error = GetTransactionStatusUphold::Error;
using Result = GetTransactionStatusUphold::Result;

// clang-format off
using GetTransactionStatusUpholdParamType = std::tuple<
    std::string,          // test name suffix
    net::HttpStatusCode,  // get transaction status endpoint status code
    std::string,          // get transaction status endpoint response body
    Result                // expected result
>;
// clang-format on

class GetTransactionStatusUphold
    : public TestWithParam<GetTransactionStatusUpholdParamType> {
 public:
  GetTransactionStatusUphold(const GetTransactionStatusUphold&) = delete;
  GetTransactionStatusUphold& operator=(const GetTransactionStatusUphold&) =
      delete;

  GetTransactionStatusUphold(GetTransactionStatusUphold&&) = delete;
  GetTransactionStatusUphold& operator=(GetTransactionStatusUphold&&) = delete;

 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  GetTransactionStatusUphold()
      : mock_ledger_client_(), mock_ledger_impl_(&mock_ledger_client_) {}

  MockLedgerClient mock_ledger_client_;
  MockLedgerImpl mock_ledger_impl_;
};

TEST_P(GetTransactionStatusUphold, Paths) {
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

  RequestFor<endpoints::GetTransactionStatusUphold>(&mock_ledger_impl_, "token",
                                                    "transaction_id")
      .Send(base::BindLambdaForTesting(
          [expected_result = expected_result](Result&& result) {
            EXPECT_EQ(result, expected_result);
          }));
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
  Endpoints,
  GetTransactionStatusUphold,
  Values(
    GetTransactionStatusUpholdParamType{
      "HTTP_200_response_not_a_dict",
      net::HTTP_OK,
      R"(
        [
          "status": "completed"
        ]
      )",
      base::unexpected(Error::kFailedToParseBody)
    },
    GetTransactionStatusUpholdParamType{
      "HTTP_200_status_wrong_case",
      net::HTTP_OK,
      R"(
        {
          "STATUS": "completed"
        }
      )",
      base::unexpected(Error::kFailedToParseBody)
    },
    GetTransactionStatusUpholdParamType{
      "HTTP_200_transaction_not_completed",
      net::HTTP_OK,
      R"(
        {
          "status": "failed"
        }
      )",
      false
    },
    GetTransactionStatusUpholdParamType{
      "HTTP_200_transaction_completed",
      net::HTTP_OK,
      R"(
        {
          "status": "completed"
        }
      )",
      true
    },
    GetTransactionStatusUpholdParamType{
      "HTTP_401_access_token_expired",
      net::HTTP_UNAUTHORIZED,
      "",
      base::unexpected(Error::kAccessTokenExpired)
    },
    GetTransactionStatusUpholdParamType{
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
