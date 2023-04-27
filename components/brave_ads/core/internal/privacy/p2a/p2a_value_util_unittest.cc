/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/privacy/p2a/p2a_value_util.h"

#include "base/test/values_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::privacy::p2a {

namespace {
constexpr char kQuestionsAsJson[] = R"(["question_1","question_2"])";
}  // namespace

TEST(BraveAdsP2AValueUtilTest, QuestionsToValue) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ(base::test::ParseJsonList(kQuestionsAsJson),
            QuestionsToValue({"question_1", "question_2"}));
}

TEST(BraveAdsP2AValueUtilTest, NoQuestionsToValue) {
  // Arrange

  // Act
  const base::Value::List list = QuestionsToValue({});

  // Assert
  EXPECT_TRUE(list.empty());
}

}  // namespace brave_ads::privacy::p2a
