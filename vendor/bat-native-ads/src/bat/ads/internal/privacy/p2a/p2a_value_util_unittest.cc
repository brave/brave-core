/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/p2a/p2a_value_util.h"

#include "base/test/values_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"  // IWYU pragma: keep

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::privacy::p2a {

namespace {

constexpr char kQuestionsAsJson[] = R"(["question_1","question_2"])";
constexpr char kNoQuestionsAsJson[] = "[]";

}  // namespace

TEST(BatAdsP2AValueUtilTest, QuestionsToValue) {
  // Arrange

  // Act
  const base::Value::List list = QuestionsToValue({"question_1", "question_2"});

  // Assert
  const base::Value value = base::test::ParseJson(kQuestionsAsJson);
  const base::Value::List* const expected_list = value.GetIfList();
  ASSERT_TRUE(expected_list);

  EXPECT_EQ(*expected_list, list);
}

TEST(BatAdsP2AValueUtilTest, NoQuestionsToValue) {
  // Arrange

  // Act
  const base::Value::List list = QuestionsToValue({});

  // Assert
  const base::Value value = base::test::ParseJson(kNoQuestionsAsJson);
  const base::Value::List* const expected_list = value.GetIfList();
  ASSERT_TRUE(expected_list);

  EXPECT_EQ(*expected_list, list);
}

}  // namespace ads::privacy::p2a
