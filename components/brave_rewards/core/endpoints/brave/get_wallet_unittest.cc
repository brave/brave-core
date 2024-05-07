/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoints/brave/get_wallet.h"

#include <string>
#include <utility>

#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/common/prefs.h"
#include "brave/components/brave_rewards/core/endpoints/request_for.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/test/rewards_engine_test.h"
#include "net/http/http_status_code.h"

// npm run test -- brave_unit_tests --filter=*GetWalletTest*

namespace brave_rewards::internal::endpoints {

class GetWalletTest : public RewardsEngineTest {
 protected:
  using Error = GetWallet::Error;

  void SetUp() override {
    std::string json = R"({
          "payment_id": "fa5dea51-6af4-44ca-801b-07b6df3dcfe4",
          "recovery_seed": "AN6DLuI2iZzzDxpzywf+IKmK1nzFRarNswbaIDI3pQg="
        })";
    engine().Get<Prefs>().SetString(prefs::kWalletBrave, std::move(json));
  }

  GetWallet::Result SendRequest(mojom::UrlResponsePtr response) {
    std::string url =
        engine()
            .Get<EnvironmentConfig>()
            .rewards_grant_url()
            .Resolve("/v4/wallets/fa5dea51-6af4-44ca-801b-07b6df3dcfe4")
            .spec();

    client().AddNetworkResultForTesting(url, mojom::UrlMethod::GET,
                                        std::move(response));

    return WaitFor<GetWallet::Result&&>([this](auto callback) {
      RequestFor<GetWallet>(engine()).Send(std::move(callback));
    });
  }
};

TEST_F(GetWalletTest, ServerError400) {
  auto response = mojom::UrlResponse::New();
  response->status_code = net::HttpStatusCode::HTTP_BAD_REQUEST;
  auto result = SendRequest(std::move(response));
  EXPECT_EQ(result, base::unexpected(Error::kInvalidRequest));
}

TEST_F(GetWalletTest, ServerError404) {
  auto response = mojom::UrlResponse::New();
  response->status_code = net::HttpStatusCode::HTTP_NOT_FOUND;
  auto result = SendRequest(std::move(response));
  EXPECT_EQ(result, base::unexpected(Error::kRewardsPaymentIDNotFound));
}

TEST_F(GetWalletTest, ServerError403) {
  auto response = mojom::UrlResponse::New();
  response->status_code = net::HttpStatusCode::HTTP_FORBIDDEN;
  auto result = SendRequest(std::move(response));
  EXPECT_EQ(result,
            base::unexpected(Error::kRequestSignatureVerificationFailure));
}

TEST_F(GetWalletTest, ServerOkNotLinked) {
  auto response = mojom::UrlResponse::New();
  response->status_code = net::HttpStatusCode::HTTP_OK;
  response->body =
      R"(
        {
          "paymentId": "368d87a3-7749-4ebb-9f3a-2882c99078c7",
          "walletProvider": {
            "id": "",
            "name": "brave"
          },
          "altcurrency": "BAT",
          "publicKey": "ae55f61fa5b2870c0ee3633004c6d7a40adb5694c73d05510d8179cec8a3403a",
          "selfCustodyAvailable": {
            "solana": true,
            "unrecongnized": true,
            "invalid": "invalid"
          }
        }
      )";
  auto result = SendRequest(std::move(response));
  EXPECT_EQ(result.value().wallet_provider, "");
  EXPECT_EQ(result.value().provider_id, "");
  EXPECT_FALSE(result.value().linked);
  EXPECT_FALSE(result.value().self_custody_available.empty());
  EXPECT_TRUE(*result.value().self_custody_available.FindBool("solana"));
  EXPECT_TRUE(*result.value().self_custody_available.FindBool("unrecongnized"));
  EXPECT_FALSE(result.value().self_custody_available.FindBool("invalid"));
}

TEST_F(GetWalletTest, ServerOKCurrentlyDisconnected) {
  auto response = mojom::UrlResponse::New();
  response->status_code = net::HttpStatusCode::HTTP_OK;
  response->body =
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
      )";
  auto result = SendRequest(std::move(response));
  EXPECT_EQ(result.value().wallet_provider, constant::kWalletUphold);
  EXPECT_EQ(result.value().provider_id, "");
  EXPECT_FALSE(result.value().linked);
}

TEST_F(GetWalletTest, ServerOkFullyLinked) {
  auto response = mojom::UrlResponse::New();
  response->status_code = net::HttpStatusCode::HTTP_OK;
  response->body =
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
      )";
  auto result = SendRequest(std::move(response));
  EXPECT_EQ(result.value().wallet_provider, constant::kWalletUphold);
  EXPECT_EQ(result.value().provider_id, "962ef3b8-bc12-4619-a349-c8083931b795");
  EXPECT_TRUE(result.value().linked);
}

}  // namespace brave_rewards::internal::endpoints
