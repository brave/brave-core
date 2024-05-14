/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/endpoint/uphold/get_capabilities/get_capabilities.h"

#include <optional>
#include <utility>

#include "brave/components/brave_rewards/core/common/environment_config.h"
#include "brave/components/brave_rewards/core/test/rewards_engine_test.h"
#include "brave/components/brave_rewards/core/uphold/uphold_capabilities.h"

namespace brave_rewards::internal {

class RewardsGetCapabilitiesTest : public RewardsEngineTest {
 protected:
  auto Request(mojom::UrlResponsePtr response) {
    auto request_url =
        engine().Get<EnvironmentConfig>().uphold_api_url().Resolve(
            "/v0/me/capabilities");

    client().AddNetworkResultForTesting(
        request_url.spec(), mojom::UrlMethod::GET, std::move(response));

    endpoint::uphold::GetCapabilities endpoint(engine());

    return WaitForValues<mojom::Result, uphold::Capabilities>(
        [&](auto callback) {
          endpoint.Request("193a77cf-02e8-4e10-8127-8a1b5a8bfece",
                           std::move(callback));
        });
  }
};

TEST_F(RewardsGetCapabilitiesTest,
       ServerReturns200OKSufficientReceivesAndSends) {
  auto make_response = [](int status_code) {
    auto response = mojom::UrlResponse::New();
    response->status_code = status_code;
    response->body = R"(
        [
          {
            "category": "permissions",
            "enabled": true,
            "key": "receives",
            "name": "Receives",
            "requirements": [],
            "restrictions": []
          },
          {
            "category": "permissions",
            "enabled": true,
            "key": "sends",
            "name": "Sends",
            "requirements": [],
            "restrictions": []
          }
        ])";
    return response;
  };

  {
    auto [result, capabilities] = Request(make_response(200));
    EXPECT_EQ(result, mojom::Result::OK);
    EXPECT_EQ(capabilities.can_receive, true);
    EXPECT_EQ(capabilities.can_send, true);
  }

  {
    auto [result, capabilities] = Request(make_response(206));
    EXPECT_EQ(result, mojom::Result::OK);
  }
}

TEST_F(RewardsGetCapabilitiesTest, ServerReturns200OKInsufficientReceives1) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 200;
  response->body = R"(
      [
        {
          "category": "permissions",
          "enabled": true,
          "key": "receives",
          "name": "Receives",
          "requirements": [
            "user-must-submit-customer-due-diligence"
          ],
          "restrictions": []
        },
        {
          "category": "permissions",
          "enabled": true,
          "key": "sends",
          "name": "Sends",
          "requirements": [],
          "restrictions": []
        }
      ])";

  auto [result, capabilities] = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::OK);
  EXPECT_EQ(capabilities.can_receive, false);
  EXPECT_EQ(capabilities.can_send, true);
}

TEST_F(RewardsGetCapabilitiesTest, ServerReturns200OKInsufficientReceives2) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 200;
  response->body = R"(
      [
        {
          "category": "permissions",
          "enabled": false,
          "key": "receives",
          "name": "Receives",
          "requirements": [],
          "restrictions": []
        },
        {
          "category": "permissions",
          "enabled": true,
          "key": "sends",
          "name": "Sends",
          "requirements": [],
          "restrictions": []
        }
      ])";

  auto [result, capabilities] = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::OK);
  EXPECT_EQ(capabilities.can_receive, false);
  EXPECT_EQ(capabilities.can_send, true);
}

TEST_F(RewardsGetCapabilitiesTest, ServerReturns200OKInsufficientSends1) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 200;
  response->body = R"(
      [
        {
          "category": "permissions",
          "enabled": true,
          "key": "receives",
          "name": "Receives",
          "requirements": [],
          "restrictions": []
        },
        {
          "category": "permissions",
          "enabled": true,
          "key": "sends",
          "name": "Sends",
          "requirements": [
            "user-must-submit-customer-due-diligence"
          ],
          "restrictions": []
        }
      ])";

  auto [result, capabilities] = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::OK);
  EXPECT_EQ(capabilities.can_receive, true);
  EXPECT_EQ(capabilities.can_send, false);
}

TEST_F(RewardsGetCapabilitiesTest, ServerReturns200OKInsufficientSends2) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 200;
  response->body = R"(
      [
        {
          "category": "permissions",
          "enabled": true,
          "key": "receives",
          "name": "Receives",
          "requirements": [],
          "restrictions": []
        },
        {
          "category": "permissions",
          "enabled": false,
          "key": "sends",
          "name": "Sends",
          "requirements": [],
          "restrictions": []
        }
      ])";

  auto [result, capabilities] = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::OK);
  EXPECT_EQ(capabilities.can_receive, true);
  EXPECT_EQ(capabilities.can_send, false);
}

TEST_F(RewardsGetCapabilitiesTest,
       ServerReturns200OKInsufficientReceivesAndSends1) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 200;
  response->body = R"(
      [
        {
          "category": "permissions",
          "enabled": true,
          "key": "receives",
          "name": "Receives",
          "requirements": [
            "user-must-submit-customer-due-diligence"
          ],
          "restrictions": []
        },
        {
          "category": "permissions",
          "enabled": true,
          "key": "sends",
          "name": "Sends",
          "requirements": [
            "user-must-submit-customer-due-diligence"
          ],
          "restrictions": []
        }
      ])";

  auto [result, capabilities] = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::OK);
  EXPECT_EQ(capabilities.can_receive, false);
  EXPECT_EQ(capabilities.can_send, false);
}

TEST_F(RewardsGetCapabilitiesTest,
       ServerReturns200OKInsufficientReceivesAndSends2) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 200;
  response->body = R"(
      [
        {
          "category": "permissions",
          "enabled": false,
          "key": "receives",
          "name": "Receives",
          "requirements": [],
          "restrictions": []
        },
        {
          "category": "permissions",
          "enabled": false,
          "key": "sends",
          "name": "Sends",
          "requirements": [],
          "restrictions": []
        }
      ])";

  auto [result, capabilities] = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::OK);
  EXPECT_EQ(capabilities.can_receive, false);
  EXPECT_EQ(capabilities.can_send, false);
}

TEST_F(RewardsGetCapabilitiesTest, ServerReturns401Unauthorized) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 401;
  auto [result, capabilities] = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::EXPIRED_TOKEN);
  EXPECT_EQ(capabilities.can_receive, std::nullopt);
  EXPECT_EQ(capabilities.can_send, std::nullopt);
}

TEST_F(RewardsGetCapabilitiesTest, ServerReturnsUnexpectedHTTPStatus) {
  auto response = mojom::UrlResponse::New();
  response->status_code = 500;

  auto [result, capabilities] = Request(std::move(response));
  EXPECT_EQ(result, mojom::Result::FAILED);
  EXPECT_EQ(capabilities.can_receive, std::nullopt);
  EXPECT_EQ(capabilities.can_send, std::nullopt);
}

}  // namespace brave_rewards::internal
