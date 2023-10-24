/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>
#include <utility>
#include <vector>

#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_rewards/core/endpoint/uphold/get_me/get_me.h"
#include "brave/components/brave_rewards/core/rewards_engine_client_mock.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl_mock.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=GetMeTest.*

using ::testing::_;

namespace brave_rewards::internal {
namespace endpoint {
namespace uphold {

class GetMeTest : public testing::Test {
 protected:
  base::test::TaskEnvironment task_environment_;
  MockRewardsEngineImpl mock_engine_impl_;
  GetMe me_{mock_engine_impl_};
};

TEST_F(GetMeTest, ServerOK) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 200;
        response->url = request->url;
        response->body = R"({
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
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<GetMeCallback> callback;
  EXPECT_CALL(callback, Run)
      .Times(1)
      .WillOnce([](mojom::Result result, const internal::uphold::User& user) {
        EXPECT_EQ(result, mojom::Result::OK);
        EXPECT_EQ(user.name, "John");
        EXPECT_EQ(user.member_id, "b34060c9-5ca3-4bdb-bc32-1f826ecea36e");
        EXPECT_EQ(user.country_id, "US");
        EXPECT_EQ(user.bat_not_allowed, false);
      });
  me_.Request("4c2b665ca060d912fec5c735c734859a06118cc8", callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(GetMeTest, ServerError401) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 401;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<GetMeCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::EXPIRED_TOKEN, _)).Times(1);
  me_.Request("4c2b665ca060d912fec5c735c734859a06118cc8", callback.Get());

  task_environment_.RunUntilIdle();
}

TEST_F(GetMeTest, ServerErrorRandom) {
  EXPECT_CALL(*mock_engine_impl_.mock_client(), LoadURL(_, _))
      .Times(1)
      .WillOnce([](mojom::UrlRequestPtr request, auto callback) {
        auto response = mojom::UrlResponse::New();
        response->status_code = 453;
        response->url = request->url;
        response->body = "";
        std::move(callback).Run(std::move(response));
      });

  base::MockCallback<GetMeCallback> callback;
  EXPECT_CALL(callback, Run(mojom::Result::FAILED, _)).Times(1);
  me_.Request("4c2b665ca060d912fec5c735c734859a06118cc8", callback.Get());

  task_environment_.RunUntilIdle();
}

}  // namespace uphold
}  // namespace endpoint
}  // namespace brave_rewards::internal
