/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/uphold/get_me/get_me.h"

#include <string>
#include <utility>

#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/test/rewards_engine_test.h"

namespace brave_rewards::internal {

class RewardsGetMeTest : public RewardsEngineTest {
 protected:
  auto Request(mojom::UrlResponsePtr response) {
    auto request_url =
        engine().Get<EnvironmentConfig>().uphold_api_url().Resolve("/v0/me");

    client().AddNetworkResultForTesting(
        request_url.spec(), mojom::UrlMethod::GET, std::move(response));

    endpoint::uphold::GetMe endpoint(engine());

    return WaitForValues<mojom::Result, uphold::User>([&](auto callback) {
      endpoint.Request("4c2b665ca060d912fec5c735c734859a06118cc8",
                       std::move(callback));
    });
  }
};

TEST_F(RewardsGetMeTest, ServerOK) {
  auto make_response = [](int status_code) {
    auto response = mojom::UrlResponse::New();
    response->status_code = status_code;
    response->body = R"(
        {
          "address": {
            "city": "Anytown",
            "line1": "123 Main Street",
            "zipCode": "12345"
          },
          "birthdate": "1971-06-22",
          "country": "US",
          "email": "john@example.com",
          "firstName": "John",
          "fullName": "John Smith",
          "id": "b34060c9-5ca3-4bdb-bc32-1f826ecea36e",
          "identityCountry": "US",
          "lastName": "Smith",
          "name": "John Smith",
          "settings": {
            "currency": "USD",
            "hasMarketingConsent": false,
            "hasNewsSubscription": false,
            "intl": {
              "dateTimeFormat": {
                "locale": "en-US"
              },
              "language": {
                "locale": "en-US"
              },
              "numberFormat": {
                "locale": "en-US"
              }
            },
            "otp": {
              "login": {
                "enabled": true
              },
              "transactions": {
                "transfer": {
                  "enabled": false
                },
                "send": {
                  "enabled": true
                },
                "withdraw": {
                  "crypto": {
                    "enabled": true
                  }
                }
              }
            },
            "theme": "vintage"
          },
          "memberAt": "2019-07-27T11:32:33.310Z",
          "state": "US-MA",
          "status": "ok",
          "type": "individual",
          "username": null,
          "verifications": {
            "termsEquities": {
              "status": "required"
            }
          },
          "balances": {
            "available": "3.15",
            "currencies": {
              "BAT": {
                "amount": "3.15",
                "balance": "12.35",
                "currency": "USD",
                "rate": "0.25521"
              }
            },
            "pending": "0.00",
            "total": "3.15"
          },
          "currencies": [
            "BAT"
          ],
          "phones": [
            {
              "e164Masked": "+XXXXXXXXX83",
              "id": "8037c7ed-fe5a-4ad2-abfd-7c941f066cab",
              "internationalMasked": "+X XXX-XXX-XX83",
              "nationalMasked": "(XXX) XXX-XX83",
              "primary": false,
              "verified": false
            }
          ],
          "tier": "other"
        })";
    return response;
  };

  {
    auto [result, user] = Request(make_response(200));
    EXPECT_EQ(result, mojom::Result::OK);
    EXPECT_EQ(user.name, "John");
    EXPECT_EQ(user.member_id, "b34060c9-5ca3-4bdb-bc32-1f826ecea36e");
    EXPECT_EQ(user.country_id, "US");
    EXPECT_EQ(user.bat_not_allowed, false);
  }

  {
    auto [result, user] = Request(make_response(206));
    EXPECT_EQ(result, mojom::Result::OK);
  }
}

TEST_F(RewardsGetMeTest, ServerError401) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 401;

  auto [result, user] = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::EXPIRED_TOKEN);
}

TEST_F(RewardsGetMeTest, ServerErrorRandom) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 453;

  auto [result, user] = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::FAILED);
}

}  // namespace brave_rewards::internal
