/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/uphold/patch_card/patch_card.h"

#include <utility>

#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/test/rewards_engine_test.h"

namespace brave_rewards::internal {

class RewardsPatchCardTest : public RewardsEngineTest {
 protected:
  auto Request(mojom::UrlResponsePtr response) {
    auto request_url =
        engine().Get<EnvironmentConfig>().uphold_api_url().Resolve(
            "/v0/me/cards/4c2b665ca060d912fec5c735c734859a06118cc8");

    client().AddNetworkResultForTesting(
        request_url.spec(), mojom::UrlMethod::PATCH, std::move(response));

    endpoint::uphold::PatchCard endpoint(engine());

    return WaitFor<mojom::Result>([&](auto callback) {
      endpoint.Request("193a77cf-02e8-4e10-8127-8a1b5a8bfece",
                       "4c2b665ca060d912fec5c735c734859a06118cc8",
                       std::move(callback));
    });
  }
};

TEST_F(RewardsPatchCardTest, ServerOK) {
  auto make_response = [](int status_code) {
    auto response = mojom::UrlResponse::New();
    response->status_code = status_code;
    response->body = R"(
        {
          "CreatedByApplicationId": "193a77cf-02e8-4e10-8127-8a1b5a8bfece",
          "address": {
            "wire": "XXXXXXXXXX"
          },
          "available": "0.00",
          "balance": "0.00",
          "currency": "BAT",
          "id": "bd91a720-f3f9-42f8-b2f5-19548004f6a7",
          "label": "Brave Browser",
          "lastTransactionAt": null,
          "settings": {
            "position": 8,
            "protected": false,
            "starred": false
          },
          "createdByApplicationClientId":
            "4c2b665ca060d912fec5c735c734859a06118cc8",
          "normalized": [
            {
              "available": "0.00",
              "balance": "0.00",
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
        })";
    return response;
  };

  {
    auto result = Request(make_response(200));
    EXPECT_EQ(result, mojom::Result::OK);
  }

  {
    auto result = Request(make_response(206));
    EXPECT_EQ(result, mojom::Result::OK);
  }
}

TEST_F(RewardsPatchCardTest, ServerError401) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 401;

  auto result = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::EXPIRED_TOKEN);
}

TEST_F(RewardsPatchCardTest, ServerErrorRandom) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 453;

  auto result = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::FAILED);
}

}  // namespace brave_rewards::internal
