/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/uphold/get_cards/get_cards.h"

#include <string>
#include <utility>

#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/test/rewards_engine_test.h"

namespace brave_rewards::internal {

class RewardsGetCardsTest : public RewardsEngineTest {
 protected:
  auto Request(mojom::UrlResponsePtr response) {
    auto request_url =
        engine().Get<EnvironmentConfig>().uphold_api_url().Resolve(
            "/v0/me/cards?q=currency:BAT");

    client().AddNetworkResultForTesting(
        request_url.spec(), mojom::UrlMethod::GET, std::move(response));

    endpoint::uphold::GetCards endpoint(engine());

    return WaitForValues<mojom::Result, std::string&&>([&](auto callback) {
      endpoint.Request("4c2b665ca060d912fec5c735c734859a06118cc8",
                       std::move(callback));
    });
  }
};

TEST_F(RewardsGetCardsTest, ServerOK) {
  auto make_response = [](int status_code) {
    auto response = mojom::UrlResponse::New();
    response->status_code = status_code;
    response->body = R"([
        {
          "CreatedByApplicationId": null,
          "address": {
            "wire": "XXXXXXXXXX"
          },
          "available": "12.35",
          "balance": "12.35",
          "currency": "BAT",
          "id": "3ed3b2c4-a715-4c01-b302-fa2681a971ea",
          "label": "Brave Browser",
          "lastTransactionAt": "2020-03-31T19:27:57.552Z",
          "settings": {
            "position": 7,
            "protected": false,
            "starred": true
          },
          "normalized": [
            {
              "available": "3.15",
              "balance": "3.15",
              "currency": "USD"
            }
          ],
          "wire": [
            {
              "accountName": "Uphold Europe Limited",
              "address": {
                "line1": "Tartu mnt 2",
                "line2": "10145 Tallinn, Estonia"
              },
              "bic": "LHVBEE22",
              "currency": "EUR",
              "iban": "EE76 7700 7710 0159 0178",
              "name": "AS LHV Pank"
            },
            {
              "accountName": "Uphold HQ, Inc.",
              "accountNumber": "XXXXXXXXXX",
              "address": {
                "line1": "1359 Broadway",
                "line2": "New York, NY 10018"
              },
              "bic": "MCBEUS33",
              "currency": "USD",
              "name": "Metropolitan Bank",
              "routingNumber": "XXXXXXXXX"
            }
          ]
        }])";
    return response;
  };

  {
    auto [result, id] = Request(make_response(200));
    EXPECT_EQ(result, mojom::Result::OK);
    EXPECT_EQ(id, "3ed3b2c4-a715-4c01-b302-fa2681a971ea");
  }

  {
    auto [result, id] = Request(make_response(206));
    EXPECT_EQ(result, mojom::Result::OK);
    EXPECT_EQ(id, "3ed3b2c4-a715-4c01-b302-fa2681a971ea");
  }
}

TEST_F(RewardsGetCardsTest, ServerPartialContent) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 206;
  response->body = R"([
      {
        "available": "12.35",
        "balance": "12.35",
        "currency": "BAT",
        "id": "3ed3b2c4-a715-4c01-b302-fa2681a971ea",
        "label": "Brave Browser"
      }])";

  auto [result, id] = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::OK);
  EXPECT_EQ(id, "3ed3b2c4-a715-4c01-b302-fa2681a971ea");
}

TEST_F(RewardsGetCardsTest, CardNotFound) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 200;
  response->body = R"([
      {
        "CreatedByApplicationId": null,
        "address": {
          "wire": "XXXXXXXXXX"
        },
        "available": "12.35",
        "balance": "12.35",
        "currency": "BAT",
        "id": "3ed3b2c4-a715-4c01-b302-fa2681a971ea",
        "label": "Test Brave Browser",
        "lastTransactionAt": "2020-03-31T19:27:57.552Z",
        "settings": {
          "position": 7,
          "protected": false,
          "starred": true
        },
        "normalized": [
          {
            "available": "3.15",
            "balance": "3.15",
            "currency": "USD"
          }
        ],
        "wire": [
          {
            "accountName": "Uphold Europe Limited",
            "address": {
              "line1": "Tartu mnt 2",
              "line2": "10145 Tallinn, Estonia"
            },
            "bic": "LHVBEE22",
            "currency": "EUR",
            "iban": "EE76 7700 7710 0159 0178",
            "name": "AS LHV Pank"
          },
          {
            "accountName": "Uphold HQ, Inc.",
            "accountNumber": "XXXXXXXXXX",
            "address": {
              "line1": "1359 Broadway",
              "line2": "New York, NY 10018"
            },
            "bic": "MCBEUS33",
            "currency": "USD",
            "name": "Metropolitan Bank",
            "routingNumber": "XXXXXXXXX"
          }
        ]
      }])";

  auto [result, id] = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::FAILED);
  EXPECT_EQ(id, "");
}

TEST_F(RewardsGetCardsTest, ServerError401) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 401;

  auto [result, id] = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::EXPIRED_TOKEN);
  EXPECT_EQ(id, "");
}

TEST_F(RewardsGetCardsTest, ServerErrorRandom) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 453;

  auto [result, id] = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::FAILED);
  EXPECT_EQ(id, "");
}

}  // namespace brave_rewards::internal
