/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
#include "brave/components/brave_rewards/core/endpoint/promotion/get_available/get_available.h"

#include <string>
#include <utility>
#include <vector>

#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/rewards_engine_client_mock.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl_mock.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=GetAvailableTest.*

using ::testing::_;
using ::testing::IsEmpty;

namespace brave_rewards::internal {
namespace endpoint {
namespace promotion {

class GetAvailableTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
  MockRewardsEngineImpl mock_engine_impl_;
  GetAvailable available_{mock_engine_impl_};
};

TEST_F(GetAvailableTest, ServerOK) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 200;
        response->url = request->url;
        response->body = R"({
             "promotions": [
               {
                 "id": "83b3b77b-e7c3-455b-adda-e476fa0656d2",
                 "createdAt": "2020-06-08T15:04:45.352584Z",
                 "expiresAt": "2020-10-08T15:04:45.352584Z",
                 "version": 5,
                 "suggestionsPerGrant": 120,
                 "approximateValue": "30",
                 "type": "ugp",
                 "available": true,
                 "platform": "desktop",
                 "publicKeys": [
                   "dvpysTSiJdZUPihius7pvGOfngRWfDiIbrowykgMi1I="
                 ],
                 "legacyClaimed": false
               }
             ]
            })";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<GetAvailableCallback> callback;
  EXPECT_CALL(callback, Run)
      .Times(1)
      .WillOnce([](mojom::Result result, std::vector<mojom::PromotionPtr> list,
                   const std::vector<std::string>& corrupted_promotions) {
        mojom::Promotion expected_promotion;
        expected_promotion.id = "83b3b77b-e7c3-455b-adda-e476fa0656d2";
        expected_promotion.created_at = 1591628685;
        expected_promotion.expires_at = 1602169485;
        expected_promotion.version = 5;
        expected_promotion.suggestions = 120;
        expected_promotion.approximate_value = 30.0;
        expected_promotion.type = mojom::PromotionType::UGP;
        expected_promotion.public_keys =
            "[\"dvpysTSiJdZUPihius7pvGOfngRWfDiIbrowykgMi1I=\"]";
        expected_promotion.legacy_claimed = false;

        EXPECT_EQ(result, mojom::Result::OK);
        EXPECT_TRUE(corrupted_promotions.empty());
        EXPECT_EQ(list.size(), 1ul);
        EXPECT_TRUE(expected_promotion.Equals(*list[0]));
      });
  available_.Request("macos", callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(GetAvailableTest, ServerError400) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 400;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<GetAvailableCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::FAILED, IsEmpty(), IsEmpty()))
      .Times(1);
  available_.Request("macos", callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(GetAvailableTest, ServerError404) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 404;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<GetAvailableCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::NOT_FOUND, IsEmpty(), IsEmpty()))
      .Times(1);
  available_.Request("macos", callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(GetAvailableTest, ServerError500) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 500;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<GetAvailableCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::FAILED, IsEmpty(), IsEmpty()))
      .Times(1);
  available_.Request("macos", callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(GetAvailableTest, ServerErrorRandom) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 453;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<GetAvailableCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::FAILED, IsEmpty(), IsEmpty()))
      .Times(1);
  available_.Request("macos", callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(GetAvailableTest, ServerWrongResponse) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 200;
        response->url = request->url;
        response->body = R"({
             "promotions": [
                {
                  "foo": 0
                }
              ]
            })";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<GetAvailableCallback> callback;
  EXPECT_CALL(callback,
              Run(mojom::Result::CORRUPTED_DATA, IsEmpty(), IsEmpty()))
      .Times(1);
  available_.Request("macos", callback.Get());

  task_environment_.RunUntilIdle();
}

}  // namespace promotion
}  // namespace endpoint
}  // namespace brave_rewards::internal
