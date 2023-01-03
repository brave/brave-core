/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/p2a/p2a.h"

#include "base/test/values_test_util.h"
#include "base/values.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BatAds*

using ::testing::Eq;

namespace ads::privacy::p2a {

namespace {

constexpr char kEventName[] = "name";
constexpr char kQuestionsAsJson[] = R"(["question_1","question_2"])";

}  // namespace

class BatAdsP2ATest : public UnitTestBase {};

TEST_F(BatAdsP2ATest, RecordEvent) {
  // Arrange
  const base::Value value = base::test::ParseJson(kQuestionsAsJson);
  const base::Value::List* const list = value.GetIfList();
  ASSERT_TRUE(list);

  EXPECT_CALL(*ads_client_mock_,
              RecordP2AEvent(kEventName, Eq(std::ref(*list))));

  // Act

  // Assert
  RecordEvent(kEventName, {"question_1", "question_2"});
}

}  // namespace ads::privacy::p2a
