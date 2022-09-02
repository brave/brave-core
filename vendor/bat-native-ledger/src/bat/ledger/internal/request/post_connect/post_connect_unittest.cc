/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <tuple>
#include <utility>

#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/internal/request/post_connect/post_connect.h"
#include "bat/ledger/internal/request/request_for.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=*PostConnect*

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::TestParamInfo;
using ::testing::TestWithParam;
using ::testing::Values;

namespace ledger::request::connect::test {

class PostConnectMock final : public request::PostConnect {
 public:
  explicit PostConnectMock(LedgerImpl* ledger) : PostConnect(ledger) {}
  ~PostConnectMock() override = default;

 private:
  const char* Path() const override { return "/v3/wallet/mock/%s/claim"; }
};

// clang-format off
using PostConnectParamType = std::tuple<
    std::string,          // test name suffix
    net::HttpStatusCode,  // connect endpoint response status code
    std::string,          // connect endpoint response body
    type::Result          // expected result
>;
// clang-format on

class PostConnect : public TestWithParam<PostConnectParamType> {
 public:
  PostConnect(const PostConnect&) = delete;
  PostConnect& operator=(const PostConnect&) = delete;

  PostConnect(PostConnect&&) = delete;
  PostConnect& operator=(PostConnect&&) = delete;

 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  PostConnect()
      : mock_ledger_client_(), mock_ledger_impl_(&mock_ledger_client_) {}

  void SetUp() override {
    const std::string wallet =
        R"(
         {
           "payment_id":"fa5dea51-6af4-44ca-801b-07b6df3dcfe4",
           "recovery_seed":"AN6DLuI2iZzzDxpzywf+IKmK1nzFRarNswbaIDI3pQg="
         }
        )";

    ON_CALL(mock_ledger_client_, GetStringState(state::kWalletBrave))
        .WillByDefault(Return(wallet));
  }

  MockLedgerClient mock_ledger_client_;
  MockLedgerImpl mock_ledger_impl_;
};

TEST_P(PostConnect, Paths) {
  const auto& [ignore, status_code, body, expected_result] = GetParam();

  ON_CALL(mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(Invoke(
          [status_code = status_code, body = body](
              type::UrlRequestPtr, client::LoadURLCallback callback) mutable {
            type::UrlResponse response;
            response.status_code = status_code;
            response.body = std::move(body);
            std::move(callback).Run(response);
          }));

  RequestFor<PostConnectMock> request{&mock_ledger_impl_};
  EXPECT_TRUE(request);

  std::move(request).Send(base::BindLambdaForTesting(
      [expected_result = expected_result](type::Result result) {
        EXPECT_EQ(result, expected_result);
      }));
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
  Endpoints,
  PostConnect,
  Values(
    PostConnectParamType{
      "00_HTTP_200",
      net::HTTP_OK,
      "",
      type::Result::LEDGER_OK
    },
    PostConnectParamType{
      "01_HTTP_400_flagged_wallet",
      net::HTTP_BAD_REQUEST,
      R"(
        {
          "message": "unable to link - unusual activity",
          "code": 400
        }
      )",
      type::Result::FLAGGED_WALLET
    },
    PostConnectParamType{
      "02_HTTP_400_mismatched_provider_account_regions",
      net::HTTP_BAD_REQUEST,
      R"(
        {
          "message": "error linking wallet: mismatched provider account regions: geo reset is different",
          "code": 400
        }
      )",
      type::Result::MISMATCHED_PROVIDER_ACCOUNT_REGIONS
    },
    PostConnectParamType{
      "03_HTTP_400_region_not_supported",
      net::HTTP_BAD_REQUEST,
      R"(
        {
          "message": "region not supported: failed to validate account: invalid country",
          "code": 400
        }
      )",
      type::Result::REGION_NOT_SUPPORTED
    },
    PostConnectParamType{
      "04_HTTP_400_unknown_message",
      net::HTTP_BAD_REQUEST,
      R"(
        {
          "message": "unknown message",
          "code": 400
        }
      )",
      type::Result::LEDGER_ERROR
    },
    PostConnectParamType{
      "05_HTTP_403_kyc_required",
      net::HTTP_FORBIDDEN,
      R"(
        {
          "message": "error linking wallet: KYC required: user kyc did not pass",
          "code": 403
        }
      )",
      type::Result::NOT_FOUND
    },
    PostConnectParamType{
      "06_HTTP_403_mismatched_provider_accounts",
      net::HTTP_FORBIDDEN,
      R"(
        {
          "message": "error linking wallet: unable to link wallets: mismatched provider accounts: wallets do not match",
          "code": 403
        }
      )",
      type::Result::MISMATCHED_PROVIDER_ACCOUNTS
    },
    PostConnectParamType{
      "07_HTTP_403_request_signature_verification_failure",
      net::HTTP_FORBIDDEN,
      R"(
        {
          "message": "request signature verification failure",
          "code": 403
        }
      )",
      type::Result::REQUEST_SIGNATURE_VERIFICATION_FAILURE
    },
    PostConnectParamType{
      "08_HTTP_403_transaction_verification_failure",
      net::HTTP_FORBIDDEN,
      R"(
        {
          "message": "error linking wallet: transaction verification failure: failed to verify transaction",
          "code": 403
        }
      )",
      type::Result::UPHOLD_TRANSACTION_VERIFICATION_FAILURE
    },
    PostConnectParamType{
      "09_HTTP_403_unknown_message",
      net::HTTP_FORBIDDEN,
      R"(
        {
          "message": "unknown message",
          "code": 403
        }
      )",
      type::Result::LEDGER_ERROR
    },
    PostConnectParamType{
      "10_HTTP_404_kyc_required",
      net::HTTP_NOT_FOUND,
      "",
      type::Result::NOT_FOUND
    },
    PostConnectParamType{
      "11_HTTP_409_device_limit_reached",
      net::HTTP_CONFLICT,
      "",
      type::Result::DEVICE_LIMIT_REACHED
    },
    PostConnectParamType{
      "12_HTTP_500_http_internal_server_error",
      net::HTTP_INTERNAL_SERVER_ERROR,
      "",
      type::Result::LEDGER_ERROR
    },
    PostConnectParamType{
      "13_HTTP_504_random_server_error",
      net::HTTP_GATEWAY_TIMEOUT,
      "",
      type::Result::LEDGER_ERROR
    }),
  [](const TestParamInfo<PostConnectParamType>& info) {
    return std::get<0>(info.param);
  }
);
// clang-format on

}  // namespace ledger::request::connect::test
