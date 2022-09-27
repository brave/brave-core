/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <tuple>
#include <utility>

#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "bat/ledger/internal/endpoints/post_wallets/post_wallets.h"
#include "bat/ledger/internal/endpoints/request_for.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=*PostWallets*

using ::testing::_;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::TestParamInfo;
using ::testing::TestWithParam;
using ::testing::Values;

namespace ledger::endpoints::test {
using Error = PostWallets::Error;
using Result = PostWallets::Result;

// clang-format off
using PostWalletsParamType = std::tuple<
    std::string,          // test name suffix
    net::HttpStatusCode,  // post create wallet endpoint response status code
    std::string,          // post create wallet endpoint response body
    Result                // expected result
>;
// clang-format on

class PostWallets : public TestWithParam<PostWalletsParamType> {
 public:
  PostWallets(const PostWallets&) = delete;
  PostWallets& operator=(const PostWallets&) = delete;

  PostWallets(PostWallets&&) = delete;
  PostWallets& operator=(PostWallets&&) = delete;

 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  PostWallets()
      : mock_ledger_client_(), mock_ledger_impl_(&mock_ledger_client_) {}

  void SetUp() override {
    const std::string wallet =
        R"(
          {
            "payment_id": "",
            "recovery_seed": "AN6DLuI2iZzzDxpzywf+IKmK1nzFRarNswbaIDI3pQg="
          }
        )";

    ON_CALL(mock_ledger_client_, GetStringState(state::kWalletBrave))
        .WillByDefault(Return(wallet));
  }

  MockLedgerClient mock_ledger_client_;
  MockLedgerImpl mock_ledger_impl_;
};

TEST_P(PostWallets, Paths) {
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

  RequestFor<endpoints::PostWallets>(&mock_ledger_impl_, "geo_country")
      .Send(base::BindLambdaForTesting(
          [expected_result = expected_result](Result&& result) {
            EXPECT_EQ(result, expected_result);
          }));
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(
  Endpoints,
  PostWallets,
  Values(
    PostWalletsParamType{
      "0_HTTP_201_success",
      net::HTTP_CREATED,
      R"(
        {
          "paymentId": "284a68ea-95ac-559a-b95c-5f07b4db0c72",
          "walletProvider": {
            "id": "",
            "name": "brave"
          },
          "altcurrency": "BAT",
          "publicKey": "7de76306129de620d01406cdd5a72c5e0ea2e427504f0faff2ba5788c81f2e76"
        }
      )",
      "284a68ea-95ac-559a-b95c-5f07b4db0c72"
    },
    PostWalletsParamType{
      "1_HTTP_400_invalid_request",
      net::HTTP_BAD_REQUEST,
      "",
      base::unexpected(Error::kInvalidRequest)
    },
    PostWalletsParamType{
      "2_HTTP_401_invalid_public_key",
      net::HTTP_UNAUTHORIZED,
      "",
      base::unexpected(Error::kInvalidPublicKey)
    },
    PostWalletsParamType{
      "3_HTTP_403_wallet_generation_disabled",
      net::HTTP_FORBIDDEN,
      "",
      base::unexpected(Error::kWalletGenerationDisabled)
    },
    PostWalletsParamType{
      "4_HTTP_409_wallet_already_exists",
      net::HTTP_CONFLICT,
      "",
      base::unexpected(Error::kWalletAlreadyExists)
    },
    PostWalletsParamType{
      "5_HTTP_500_unexpected_error",
      net::HTTP_INTERNAL_SERVER_ERROR,
      "",
      base::unexpected(Error::kUnexpectedError)
    },
    PostWalletsParamType{
      "6_HTTP_503_unexpected_status_code",
      net::HTTP_SERVICE_UNAVAILABLE,
      "",
      base::unexpected(Error::kUnexpectedStatusCode)
    }),
  [](const TestParamInfo<PostWalletsParamType>& info) {
    return std::get<0>(info.param);
  }
);
// clang-format on

}  // namespace ledger::endpoints::test
