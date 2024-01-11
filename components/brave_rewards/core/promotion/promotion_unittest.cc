/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>

#include "base/containers/flat_map.h"
#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/common/time_util.h"
#include "brave/components/brave_rewards/core/database/database_mock.h"
#include "brave/components/brave_rewards/core/promotion/promotion.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"
#include "brave/components/brave_rewards/core/rewards_engine_client_mock.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl_mock.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PromotionTest.*

using ::testing::_;
using ::testing::Field;
using ::testing::HasSubstr;
using ::testing::Pointee;

namespace brave_rewards::internal::promotion {

class PromotionTest : public testing::Test {
 protected:
  void SetUp() override {
    ON_CALL(*mock_engine_impl_.mock_client(),
            GetStringState(state::kWalletBrave, _))
        .WillByDefault([](const std::string&, auto callback) {
          std::move(callback).Run(R"(
            {
              "payment_id":"fa5dea51-6af4-44ca-801b-07b6df3dcfe4",
              "recovery_seed":"AN6DLuI2iZzzDxpzywf+IKmK1nzFRarNswbaIDI3pQg="
            }
          )");
        });

    ON_CALL(*mock_engine_impl_.mock_client(), GetClientInfo(_))
        .WillByDefault([](auto callback) {
          std::move(callback).Run(mojom::ClientInfo::New());
        });
  }

  base::test::TaskEnvironment task_environment_;
  MockRewardsEngineImpl mock_engine_impl_;
  Promotion promotion_{mock_engine_impl_};
};

TEST_F(PromotionTest, LegacyPromotionIsNotOverwritten) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(),
              LoadURL(Pointee(Field(&mojom::UrlRequest::url,
                                    HasSubstr("/v1/promotions"
                                              "?migrate=true&platform=windows&"
                                              "paymentId=fa5dea51-"
                                              "6af4-44ca-801b-07b6df3dcfe4"))),
                      _))
      .Times(2)
      .WillRepeatedly([](mojom::UrlRequestPtr, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 200;
        response->body = R"(
          {
            "promotions": [
              {
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
              }
            ]
          }
        )";
        std::move(callback).Run(std::move(response));
      });

  bool inserted = false;
  EXPECT_CALL(*mock_engine_impl_.mock_database(), GetAllPromotions(_))
      .Times(2)
      .WillRepeatedly([&inserted](auto callback) {
        base::flat_map<std::string, mojom::PromotionPtr> map;
        if (inserted) {
          std::string id = "36baa4c3-f92d-4121-b6d9-db44cb273a02";
          auto promotion = mojom::Promotion::New();
          promotion->id = id;
          promotion->public_keys =
              "[\"vNnt88kCh650dFFHt+48SS4d4skQ2FYSxmmlzmKDgkE=\"]";
          promotion->legacy_claimed = true;
          promotion->status = mojom::PromotionStatus::ATTESTED;
          map.emplace(std::move(id), std::move(promotion));
        }

        callback(std::move(map));
      });

  // to suppress the Fetch(base::DoNothing()) calls
  // triggered by Promotion::OnLastCheckTimerElapsed()
  EXPECT_CALL(*mock_engine_impl_.mock_client(),
              GetUint64State(state::kPromotionLastFetchStamp, _))
      .Times(2)
      .WillRepeatedly([](const std::string&, auto callback) {
        std::move(callback).Run(util::GetCurrentTimeStamp());
      });

  EXPECT_CALL(*mock_engine_impl_.mock_database(), SavePromotion(_, _)).Times(1);

  base::MockCallback<FetchPromotionsCallback> callback;
  EXPECT_CALL(callback, Run).Times(2);

  // to avoid fulfilling the fetch request from database in Promotion::Fetch()
  is_testing = true;
  promotion_.Fetch(callback.Get());
  task_environment_.RunUntilIdle();

  inserted = true;
  promotion_.Fetch(callback.Get());
  task_environment_.RunUntilIdle();
  is_testing = false;
}

}  // namespace brave_rewards::internal::promotion
