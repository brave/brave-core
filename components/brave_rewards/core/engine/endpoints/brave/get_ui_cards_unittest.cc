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
        "url": "$banner-url",
        "offer_id": "$banner-offer"
      },
      "items": [{
        "title": "$title",
        "description": "$description",
        "url": "$url",
        "thumbnail": "$thumbnail",
        "offer_id": "$offer",
        "events": [
          {"type": "hover", "trigger": "hover", "dwell_ms": 500,
           "once_per_load": true},
          {"type": "click", "trigger": "click"}
        ]
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

  EXPECT_EQ(community_card->items[0]->offer_id, "");
  EXPECT_TRUE(community_card->items[0]->events.empty());
  EXPECT_EQ(promo_card->banner->offer_id, "$banner-offer");
  EXPECT_TRUE(promo_card->banner->events.empty());
  auto& events = promo_card->items[0]->events;
  EXPECT_EQ(promo_card->items[0]->offer_id, "$offer");
  ASSERT_EQ(events.size(), static_cast<size_t>(2));
  EXPECT_EQ(events[0]->type, "hover");
  EXPECT_EQ(events[0]->trigger, mojom::OfferEventTrigger::kHover);
  EXPECT_EQ(events[0]->dwell_ms, 500);
  EXPECT_TRUE(events[0]->once_per_load);
  EXPECT_EQ(events[1]->type, "click");
  EXPECT_EQ(events[1]->trigger, mojom::OfferEventTrigger::kClick);
  EXPECT_EQ(events[1]->dwell_ms, 0);
  EXPECT_FALSE(events[1]->once_per_load);
}

TEST_F(RewardsGetUICardsTest, ReadsOnlyValidOfferEvents) {
  auto response = mojom::UrlResponse::New();
  response->status_code = net::HttpStatusCode::HTTP_OK;
  response->body = R"({
    "community-card": [{
      "url": "$url",
      "offer_id": "$offer",
      "events": [
        "not-a-dict",
        {"type": "no-trigger"},
        {"trigger": "click"},
        {"type": "view", "trigger": "view"},
        {"type": "click", "trigger": "click", "dwell_ms": -5}
      ]
    }]
  })";

  auto result = SendRequest(std::move(response));
  ASSERT_EQ(result->size(), static_cast<size_t>(1));
  auto& events = result->at(0)->items[0]->events;
  ASSERT_EQ(events.size(), static_cast<size_t>(1));
  EXPECT_EQ(events[0]->type, "click");
  EXPECT_EQ(events[0]->trigger, mojom::OfferEventTrigger::kClick);
  EXPECT_EQ(events[0]->dwell_ms, 0);
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
