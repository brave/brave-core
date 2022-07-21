/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/p2a/p2a.h"

#include "bat/ads/internal/base/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace privacy {
namespace p2a {

namespace {

constexpr char kName[] = "name";
constexpr char kExpectedValue[] = R"~(["question_1","question_2"])~";

}  // namespace

class BatAdsP2ATest : public UnitTestBase {
 protected:
  BatAdsP2ATest() = default;

  ~BatAdsP2ATest() override = default;
};

TEST_F(BatAdsP2ATest, RecordEvent) {
  // Arrange
  EXPECT_CALL(*ads_client_mock_, RecordP2AEvent(kName, kExpectedValue));

  // Act

  // Assert
  RecordEvent(kName, {"question_1", "question_2"});
}

}  // namespace p2a
}  // namespace privacy
}  // namespace ads
