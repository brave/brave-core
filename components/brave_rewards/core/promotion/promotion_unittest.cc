/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/database/database_mock.h"
#include "brave/components/brave_rewards/core/endpoint/promotion/promotions_util.h"
#include "brave/components/brave_rewards/core/ledger.h"
#include "brave/components/brave_rewards/core/ledger_client_mock.h"
#include "brave/components/brave_rewards/core/ledger_impl_mock.h"
#include "brave/components/brave_rewards/core/promotion/promotion.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PromotionTest.*

using ::testing::_;

namespace ledger {
namespace promotion {

std::string GetResponse(const std::string& url) {
  std::map<std::string, std::string> response;

  // Fetch promotions
  response.insert(std::make_pair(
      endpoint::promotion::GetServerUrl(
          "/v1/promotions"
          "?migrate=true&paymentId=fa5dea51-6af4-44ca-801b-07b6df3dcfe4"
          "&platform="),
      R"({
      "promotions":[{
        "id":"36baa4c3-f92d-4121-b6d9-db44cb273a02",
        "createdAt":"2019-10-30T23:17:15.681226Z",
        "expiresAt":"2020-02-29T23:17:15.681226Z",
        "version":5,
        "suggestionsPerGrant":70,
        "approximateValue":"17.5",
        "type":"ugp",
        "available":true,
        "platform":"desktop",
        "publicKeys":["vNnt88kCh650dFFHt+48SS4d4skQ2FYSxmmlzmKDgkE="],
        "legacyClaimed":false
      }]})"));

  return response[url];
}

class PromotionTest : public testing::Test {
 protected:
  void SetUp() override {
    ON_CALL(*mock_ledger_impl_.mock_rewards_service(),
            GetStringState(state::kWalletBrave, _))
        .WillByDefault([](const std::string&, auto callback) {
          std::string wallet = R"(
            {
              "payment_id":"fa5dea51-6af4-44ca-801b-07b6df3dcfe4",
              "recovery_seed":"AN6DLuI2iZzzDxpzywf+IKmK1nzFRarNswbaIDI3pQg="
            }
          )";
          std::move(callback).Run(std::move(wallet));
        });

    ON_CALL(*mock_ledger_impl_.mock_rewards_service(), LoadURL(_, _))
        .WillByDefault(
            [](mojom::UrlRequestPtr request, LoadURLCallback callback) {
              auto response = mojom::UrlResponse::New();
              response->status_code = 200;
              response->url = request->url;
              response->body = GetResponse(request->url);
              std::move(callback).Run(std::move(response));
            });
  }

  base::test::TaskEnvironment task_environment_;
  MockLedgerImpl mock_ledger_impl_;
  Promotion promotion_{&mock_ledger_impl_};
};

TEST_F(PromotionTest, DISABLED_LegacyPromotionIsNotOverwritten) {
  bool inserted = false;
  ON_CALL(*mock_ledger_impl_.mock_database(), GetAllPromotions(_))
      .WillByDefault([&inserted](ledger::GetAllPromotionsCallback callback) {
        auto promotion = mojom::Promotion::New();
        base::flat_map<std::string, mojom::PromotionPtr> map;
        if (inserted) {
          const std::string id = "36baa4c3-f92d-4121-b6d9-db44cb273a02";
          promotion->id = id;
          promotion->public_keys =
              "[\"vNnt88kCh650dFFHt+48SS4d4skQ2FYSxmmlzmKDgkE=\"]";
          promotion->legacy_claimed = true;
          promotion->status = mojom::PromotionStatus::ATTESTED;
          map.insert(std::make_pair(id, std::move(promotion)));
        }

        callback(std::move(map));
      });

  EXPECT_CALL(*mock_ledger_impl_.mock_database(), SavePromotion(_, _)).Times(1);

  promotion_.Fetch(base::DoNothing());
  inserted = true;
  promotion_.Fetch(base::DoNothing());
}

}  // namespace promotion
}  // namespace ledger
