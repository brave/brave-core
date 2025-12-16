/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/engine/endpoints/brave/get_ui_cards.h"

#include <utility>

#include "brave/components/brave_rewards/core/engine/test/rewards_engine_test.h"
#include "brave/components/brave_rewards/core/engine/util/environment_config.h"
#include "net/http/http_status_code.h"

namespace brave_rewards::internal::endpoints {

class RewardsGetUICardsTest : public RewardsEngineTest {
 protected:
  GetUICards::Result SendRequest(mojom::UrlResponsePtr response) {
    auto url = engine().Get<EnvironmentConfig>().rewards_api_url().Resolve(
        "/v1/cards");

    client().AddNetworkResultForTesting(url.spec(), mojom::UrlMethod::GET,
                                        std::move(response));

    GetUICards endpoint(engine());
    return WaitFor<GetUICards::Result>(
        [&](auto callback) { endpoint.Request(std::move(callback)); });
  }
};

TEST_F(RewardsGetUICardsTest, ExpectedResponse) {
  auto response = mojom::UrlResponse::New();
  response->status_code = net::HttpStatusCode::HTTP_OK;
  response->body = R"({
    "community-card": [{
      "title": "$title",
      "description": "$description",
      "url": "$url",
      "thumbnail": "$thumbnail"
    }],
    "partner-promo-card": {
      "title": "$card-title",
      "section": "explore",
      "order": 1,
      "banner": {
        "image": "$banner-image",
        "url": "$banner-url"
      },
      "items": [{
        "title": "$title",
        "description": "$description",
        "url": "$url",
        "thumbnail": "$thumbnail"
      }]
    }
  })";

  auto result = SendRequest(std::move(response));
  ASSERT_EQ(result->size(), static_cast<size_t>(2));

  auto& community_card = result->at(0);
  EXPECT_EQ(community_card->name, "community-card");
  EXPECT_EQ(community_card->title, "");
  EXPECT_EQ(community_card->section, "");
  EXPECT_EQ(community_card->order, 0);
  ASSERT_EQ(community_card->items.size(), static_cast<size_t>(1));
  EXPECT_EQ(community_card->items[0]->title, "$title");
  EXPECT_EQ(community_card->items[0]->description, "$description");
  EXPECT_EQ(community_card->items[0]->url, "$url");
  EXPECT_EQ(community_card->items[0]->thumbnail, "$thumbnail");

  auto& promo_card = result->at(1);
  EXPECT_EQ(promo_card->name, "partner-promo-card");
  EXPECT_EQ(promo_card->title, "$card-title");
  EXPECT_EQ(promo_card->section, "explore");
  EXPECT_EQ(promo_card->order, 1);
  ASSERT_TRUE(promo_card->banner);
  EXPECT_EQ(promo_card->banner->image, "$banner-image");
  EXPECT_EQ(promo_card->banner->url, "$banner-url");
  ASSERT_EQ(promo_card->items.size(), static_cast<size_t>(1));
  EXPECT_EQ(promo_card->items[0]->title, "$title");
  EXPECT_EQ(promo_card->items[0]->description, "$description");
  EXPECT_EQ(promo_card->items[0]->url, "$url");
  EXPECT_EQ(promo_card->items[0]->thumbnail, "$thumbnail");
}

TEST_F(RewardsGetUICardsTest, ErrorStatus) {
  auto response = mojom::UrlResponse::New();
  response->status_code = net::HttpStatusCode::HTTP_NOT_FOUND;
  auto result = SendRequest(std::move(response));
  EXPECT_FALSE(result);
}

TEST_F(RewardsGetUICardsTest, BadJSON) {
  auto response = mojom::UrlResponse::New();
  response->status_code = net::HttpStatusCode::HTTP_OK;
  response->body = "bad json";
  auto result = SendRequest(std::move(response));
  EXPECT_FALSE(result);
}

TEST_F(RewardsGetUICardsTest, EmptyBody) {
  auto response = mojom::UrlResponse::New();
  response->status_code = net::HttpStatusCode::HTTP_OK;
  auto result = SendRequest(std::move(response));
  EXPECT_FALSE(result);
}

}  // namespace brave_rewards::internal::endpoints
