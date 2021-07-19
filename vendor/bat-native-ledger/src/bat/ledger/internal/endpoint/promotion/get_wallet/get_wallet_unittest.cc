/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/endpoint/promotion/get_wallet/get_wallet.h"

#include <memory>
#include <string>
#include <tuple>

#include "base/test/task_environment.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/ledger_client_mock.h"
#include "bat/ledger/internal/ledger_impl_mock.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=*GetWalletTest*

using ::testing::_;
using ::testing::TestParamInfo;
using ::testing::TestWithParam;
using ::testing::Values;

namespace ledger {
namespace endpoint {
namespace promotion {

template <typename ParamType>
std::string NameSuffixGenerator(const TestParamInfo<ParamType>& info) {
  return std::get<0>(info.param);
}

// clang-format off
using GetWalletParamType = std::tuple<
    std::string,        // test name suffix
    type::UrlResponse,  // Rewards Get Wallet response
    type::Result,       // expected result
    std::string,        // expected custodian
    bool                // expected linked
>;

struct GetWalletTest : TestWithParam<GetWalletParamType> {
 private:
  base::test::TaskEnvironment scoped_task_environment_;

 protected:
  std::unique_ptr<MockLedgerClient> mock_ledger_client_;
  std::unique_ptr<MockLedgerImpl> mock_ledger_impl_;
  std::unique_ptr<GetWallet> get_wallet_;

  GetWalletTest()
      : mock_ledger_client_{std::make_unique<MockLedgerClient>()},
        mock_ledger_impl_{std::make_unique<MockLedgerImpl>(
            mock_ledger_client_.get())},
        get_wallet_{std::make_unique<GetWallet>(mock_ledger_impl_.get())} {}
};

INSTANTIATE_TEST_SUITE_P(
  Endpoints,
  GetWalletTest,
  Values(
    GetWalletParamType{
      "ServerError400",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_BAD_REQUEST,
        {},
        {}
      },
      type::Result::LEDGER_ERROR,
      "",
      false
    },
    GetWalletParamType{
      "ServerError404",
      type::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_NOT_FOUND,
        {},
        {}
      },
      type::Result::LEDGER_ERROR,
      "",
      false
    },
    GetWalletParamType{
      "ServerOK_not_linked",
      type::UrlResponse{
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
      type::Result::LEDGER_OK,
      "",
      false
    },
    GetWalletParamType{
      "ServerOK_was_linked_but_currently_disconnected",
      type::UrlResponse{
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
      type::Result::LEDGER_OK,
      constant::kWalletUphold,
      false
    },
    GetWalletParamType{
      "ServerOK_fully_linked",
      type::UrlResponse{
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
      type::Result::LEDGER_OK,
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

  ON_CALL(*mock_ledger_client_, LoadURL(_, _))
      .WillByDefault(
          [&](type::UrlRequestPtr, client::LoadURLCallback callback) {
            callback(rewards_services_get_wallet_response);
          });

  get_wallet_->Request(
      [&](type::Result result, const std::string& custodian, bool linked) {
        EXPECT_EQ(result, expected_result);
        EXPECT_EQ(custodian, expected_custodian);
        EXPECT_EQ(linked, expected_linked);
      });
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace ledger
