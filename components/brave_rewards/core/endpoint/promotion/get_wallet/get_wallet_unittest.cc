/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/promotion/get_wallet/get_wallet.h"

#include <string>
#include <tuple>
#include <utility>

#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/ledger_client_mock.h"
#include "brave/components/brave_rewards/core/ledger_impl_mock.h"
#include "brave/components/brave_rewards/core/test/mock_ledger_test.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=*GetWalletTest*

using ::testing::_;
using ::testing::MockFunction;
using ::testing::TestParamInfo;
using ::testing::Values;
using ::testing::WithParamInterface;

namespace brave_rewards::internal::endpoint::promotion {

template <typename ParamType>
std::string NameSuffixGenerator(const TestParamInfo<ParamType>& info) {
  return std::get<0>(info.param);
}

// clang-format off
using GetWalletParamType = std::tuple<
    std::string,        // test name suffix
    mojom::UrlResponse,  // Rewards Get Wallet response
    mojom::Result,       // expected result
    std::string,        // expected custodian
    bool                // expected linked
>;

class GetWalletTest : public MockLedgerTest,
                      public WithParamInterface<GetWalletParamType> {
 protected:
  GetWallet get_wallet_;
};

INSTANTIATE_TEST_SUITE_P(
  Endpoints,
  GetWalletTest,
  Values(
    GetWalletParamType{
      "ServerError400",
      mojom::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_BAD_REQUEST,
        {},
        {}
      },
      mojom::Result::LEDGER_ERROR,
      "",
      false
    },
    GetWalletParamType{
      "ServerError404",
      mojom::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_NOT_FOUND,
        {},
        {}
      },
      mojom::Result::LEDGER_ERROR,
      "",
      false
    },
    GetWalletParamType{
      "ServerOK_not_linked",
      mojom::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"(
        {
            "paymentId": "368d87a3-7749-4ebb-9f3a-2882c99078c7",
            "walletProvider": {
                "id": "",
                "name": "brave"
            },
            "altcurrency": "BAT",
            "publicKey": "ae55f61fa5b2870c0ee3633004c6d7a40adb5694c73d05510d8179cec8a3403a"
        }
        )",
        {}
      },
      mojom::Result::LEDGER_OK,
      "",
      false
    },
    GetWalletParamType{
      "ServerOK_was_linked_but_currently_disconnected",
      mojom::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"(
        {
            "paymentId": "368d87a3-7749-4ebb-9f3a-2882c99078c7",
            "depositAccountProvider": {
                "name": "uphold",
                "id": "",
                "linkingId": "4668ba96-7129-5e85-abdc-0c144ab78834"
            },
            "walletProvider": {
                "id": "",
                "name": "brave"
            },
            "altcurrency": "BAT",
            "publicKey": "ae55f61fa5b2870c0ee3633004c6d7a40adb5694c73d05510d8179cec8a3403a"
        }
        )",
        {}
      },
      mojom::Result::LEDGER_OK,
      constant::kWalletUphold,
      false
    },
    GetWalletParamType{
      "ServerOK_fully_linked",
      mojom::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_OK,
        R"(
        {
            "paymentId": "368d87a3-7749-4ebb-9f3a-2882c99078c7",
            "depositAccountProvider": {
                "name": "uphold",
                "id": "962ef3b8-bc12-4619-a349-c8083931b795",
                "linkingId": "4668ba96-7129-5e85-abdc-0c144ab78834"
            },
            "walletProvider": {
                "id": "",
                "name": "brave"
            },
            "altcurrency": "BAT",
            "publicKey": "ae55f61fa5b2870c0ee3633004c6d7a40adb5694c73d05510d8179cec8a3403a"
        }
        )",
        {}
      },
      mojom::Result::LEDGER_OK,
      constant::kWalletUphold,
      true
    }),
  NameSuffixGenerator<GetWalletParamType>
);
// clang-format on

TEST_P(GetWalletTest, Paths) {
  const auto& params = GetParam();
  const auto& rewards_services_get_wallet_response = std::get<1>(params);
  const auto expected_result = std::get<2>(params);
  const auto& expected_custodian = std::get<3>(params);
  const auto expected_linked = std::get<4>(params);

  EXPECT_CALL(mock_ledger().mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([&](mojom::UrlRequestPtr, auto callback) {
        std::move(callback).Run(
            mojom::UrlResponse::New(rewards_services_get_wallet_response));
      });

  MockFunction<GetWalletCallback> callback;
  EXPECT_CALL(callback,
              Call(expected_result, expected_custodian, expected_linked))
      .Times(1);
  get_wallet_.Request(callback.AsStdFunction());

  task_environment_.RunUntilIdle();
}

}  // namespace brave_rewards::internal::endpoint::promotion
