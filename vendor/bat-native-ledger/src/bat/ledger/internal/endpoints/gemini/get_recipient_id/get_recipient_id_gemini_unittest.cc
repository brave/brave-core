/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <tuple>
#include <utility>

#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "bat/ledger/internal/endpoints/gemini/get_recipient_id/get_recipient_id_gemini.h"
#include "bat/ledger/internal/endpoints/request_for.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=*GetRecipientIDGemini*

using ::testing::_;
using ::testing::Invoke;
using ::testing::TestParamInfo;
using ::testing::TestWithParam;
using ::testing::Values;

namespace ledger::endpoints::test {
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
 public:
  GetRecipientIDGemini(const GetRecipientIDGemini&) = delete;
  GetRecipientIDGemini& operator=(const GetRecipientIDGemini&) = delete;

  GetRecipientIDGemini(GetRecipientIDGemini&&) = delete;
  GetRecipientIDGemini& operator=(GetRecipientIDGemini&&) = delete;

 private:
  base::test::TaskEnvironment task_environment_;

 protected:
  GetRecipientIDGemini()
      : mock_ledger_client_(), mock_ledger_impl_(&mock_ledger_client_) {}

  MockLedgerClient mock_ledger_client_;
  MockLedgerImpl mock_ledger_impl_;
};

TEST_P(GetRecipientIDGemini, Paths) {
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

  RequestFor<endpoints::GetRecipientIDGemini>(&mock_ledger_impl_, "token")
      .Send(base::BindLambdaForTesting(
          [expected_result = expected_result](Result&& result) {
            EXPECT_EQ(result, expected_result);
          }));
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

}  // namespace ledger::endpoints::test
