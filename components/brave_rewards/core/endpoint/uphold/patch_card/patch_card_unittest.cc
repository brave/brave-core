/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>
#include <vector>

#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/endpoint/uphold/patch_card/patch_card.h"
#include "brave/components/brave_rewards/core/rewards_engine_client_mock.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl_mock.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=PatchCardTest.*

using ::testing::_;

namespace brave_rewards::internal {
namespace endpoint {
namespace uphold {

class PatchCardTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
  MockRewardsEngineImpl mock_engine_impl_;
  PatchCard card_{mock_engine_impl_};
};

TEST_F(PatchCardTest, ServerOK) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 200;
        response->url = request->url;
        response->body = R"({
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
             "createdByApplicationClientId": "4c2b665ca060d912fec5c735c734859a06118cc8",
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
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<PatchCardCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::OK)).Times(1);
  card_.Request("193a77cf-02e8-4e10-8127-8a1b5a8bfece",
                "4c2b665ca060d912fec5c735c734859a06118cc8", callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(PatchCardTest, ServerError401) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 401;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<PatchCardCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::EXPIRED_TOKEN)).Times(1);
  card_.Request("193a77cf-02e8-4e10-8127-8a1b5a8bfece",
                "4c2b665ca060d912fec5c735c734859a06118cc8", callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(PatchCardTest, ServerErrorRandom) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 453;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<PatchCardCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::FAILED)).Times(1);
  card_.Request("193a77cf-02e8-4e10-8127-8a1b5a8bfece",
                "4c2b665ca060d912fec5c735c734859a06118cc8", callback.Get());

  task_environment_.RunUntilIdle();
}

}  // namespace uphold
}  // namespace endpoint
}  // namespace brave_rewards::internal
