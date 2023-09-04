/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <tuple>
#include <utility>

#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/endpoints/brave/get_wallet.h"
#include "brave/components/brave_rewards/core/endpoints/request_for.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl_mock.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=*GetWalletTest*

using ::testing::_;
using ::testing::TestWithParam;
using ::testing::Values;

namespace brave_rewards::internal::endpoints::test {
using Error = GetWallet::Error;
using Result = GetWallet::Result;

// clang-format off
using GetWalletParamType = std::tuple<
    std::string,         // test name suffix
    mojom::UrlResponse,  // Rewards Get Wallet response
    Result               // expected result
>;

class GetWalletTest : public TestWithParam<GetWalletParamType> {
 protected:
  void SetUp() override {
    ON_CALL(*mock_engine_impl_.mock_client(),
            GetStringState(state::kWalletBrave, _))
        .WillByDefault([](const std::string&, auto callback) {
          std::move(callback).Run(R"({
            "payment_id": "fa5dea51-6af4-44ca-801b-07b6df3dcfe4",
            "recovery_seed": "AN6DLuI2iZzzDxpzywf+IKmK1nzFRarNswbaIDI3pQg="
          })");
        });
  }

  base::test::TaskEnvironment task_environment_;
  MockRewardsEngineImpl mock_engine_impl_;
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
      base::unexpected(Error::kInvalidRequest)
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
      base::unexpected(Error::kRewardsPaymentIDNotFound)
    },
    GetWalletParamType{
      "ServerError403",
      mojom::UrlResponse{
        {},
        {},
        net::HttpStatusCode::HTTP_FORBIDDEN,
        {},
        {}
      },
      base::unexpected(Error::kRequestSignatureVerificationFailure)
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
      {}
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
      std::pair{constant::kWalletUphold, false}
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
      std::pair{constant::kWalletUphold, true}
    }),
    [](const auto& info) {
      return std::get<0>(info.param);
    }
);
// clang-format on

TEST_P(GetWalletTest, Paths) {
  const auto& [ignore, response, result] = GetParam();

  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([&](mojom::UrlRequestPtr, auto callback) {
        std::move(callback).Run(response.Clone());
      });

  base::MockCallback<base::OnceCallback<void(Result&&)>> callback;
  EXPECT_CALL(callback, Run(Result(result))).Times(1);
  RequestFor<GetWallet>(mock_engine_impl_).Send(callback.Get());

  task_environment_.RunUntilIdle();
}

}  // namespace brave_rewards::internal::endpoints::test
