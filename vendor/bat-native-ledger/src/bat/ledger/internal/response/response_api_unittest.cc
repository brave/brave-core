/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/response/response_api.h"
#include "bat/ledger/ledger.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=ResponseApiUtilTest.*

namespace braveledger_response_util {

class ResponseApiUtilTest : public testing::Test {
};

TEST(ResponseApiUtilTest, ParseParametersWrongListValues) {
  ledger::RewardsParameters result;

  ledger::UrlResponse response;
  response.status_code = net::HTTP_OK;
  response.body = R"(
   {
     "batRate": 0.2476573499489187,
     "autocontribute": {
       "choices": [
         "5",
         "10",
         "15",
         "20",
         "25",
         "50",
         "100"
       ],
       "defaultChoice": 20
     },
     "tips": {
       "defaultTipChoices": [
         "1",
         "10",
         "100"
       ],
       "defaultMonthlyChoices": [
         "1",
         "10",
         "100"
       ]
     }
   }
  )";

  ParseParameters(response, &result);
  ASSERT_TRUE(result.tip_choices.empty());
  ASSERT_TRUE(result.monthly_tip_choices.empty());
  ASSERT_TRUE(result.auto_contribute_choices.empty());
}

TEST(ResponseApiUtilTest, ParseParametersIntListValues) {
  ledger::RewardsParameters result;

  ledger::UrlResponse response;
  response.status_code = net::HTTP_OK;
  response.body = R"(
   {
     "batRate": 0.2476573499489187,
     "autocontribute": {
       "choices": [
         5,
         10,
         15,
         20,
         25,
         50,
         100
       ],
       "defaultChoice": 20
     },
     "tips": {
       "defaultTipChoices": [
         1,
         10,
         100
       ],
       "defaultMonthlyChoices": [
         1,
         10,
         100
       ]
     }
   }
  )";

  ParseParameters(response, &result);
  ASSERT_EQ(result.auto_contribute_choices.size(), 7ul);
  ASSERT_EQ(result.tip_choices.size(), 3ul);
  ASSERT_EQ(result.monthly_tip_choices.size(), 3ul);
}

TEST(ResponseApiUtilTest, ParseParametersDoubleListValues) {
  ledger::RewardsParameters result;

  ledger::UrlResponse response;
  response.status_code = net::HTTP_OK;
  response.body = R"(
   {
     "batRate": 0.2476573499489187,
     "autocontribute": {
       "choices": [
         5.0,
         10.0,
         15.0,
         20.0,
         25.0,
         50.5,
         100.0
       ],
       "defaultChoice": 20
     },
     "tips": {
       "defaultTipChoices": [
         1.0,
         10.0,
         100.0
       ],
       "defaultMonthlyChoices": [
         1.0,
         10.0,
         100.0
       ]
     }
   }
  )";

  ParseParameters(response, &result);
  ASSERT_EQ(result.auto_contribute_choices.size(), 7ul);
  ASSERT_EQ(result.tip_choices.size(), 3ul);
  ASSERT_EQ(result.monthly_tip_choices.size(), 3ul);
}

}  // namespace braveledger_response_util
