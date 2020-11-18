/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_targeting/behavioral/purchase_intent_classifier/purchase_intent_classifier.h"

#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace ad_targeting {
namespace behavioral {

class BatAdsPurchaseIntentClassifierTest : public UnitTestBase {
 protected:
  BatAdsPurchaseIntentClassifierTest() = default;

  ~BatAdsPurchaseIntentClassifierTest() override = default;
};

TEST_F(BatAdsPurchaseIntentClassifierTest,
    InitializeClassifier) {
  PurchaseIntentClassifier purchase_intent_classifier;
  purchase_intent_classifier.LoadUserModelForLocale("en-US");

  EXPECT_TRUE(purchase_intent_classifier.IsInitialized());
}

TEST_F(BatAdsPurchaseIntentClassifierTest,
    ExtractSignalAndMatchFunnelSite) {
  // Arrange
  PurchaseIntentClassifier purchase_intent_classifier;
  purchase_intent_classifier.LoadUserModelForLocale("en-US");

  const std::string url = "https://www.brave.com/test?foo=bar";
  const std::string last_url = "https://www.foobar.com";

  // Act
  const PurchaseIntentSignalInfo info =
      purchase_intent_classifier.MaybeExtractIntentSignal(url, last_url);

  // Assert
  const PurchaseIntentSegmentList expected_segments({
    "segment 2",
    "segment 3"
  });

  EXPECT_EQ(expected_segments, info.segments);
  EXPECT_EQ(1, info.weight);
}

TEST_F(BatAdsPurchaseIntentClassifierTest,
    ExtractSignalAndMatchSegmentKeyword) {
  // Arrange
  PurchaseIntentClassifier purchase_intent_classifier;
  purchase_intent_classifier.LoadUserModelForLocale("en-US");

  const std::string url = "https://duckduckgo.com/?q=segment+keyword+1&foo=bar";
  const std::string last_url = "https://www.foobar.com";

  // Act
  const PurchaseIntentSignalInfo info =
      purchase_intent_classifier.MaybeExtractIntentSignal(url, last_url);

  // Assert
  const PurchaseIntentSegmentList expected_segments({
    "segment 1"
  });

  EXPECT_EQ(expected_segments, info.segments);
  EXPECT_EQ(1, info.weight);
}

TEST_F(BatAdsPurchaseIntentClassifierTest,
    ExtractSignalAndMatchFunnelKeyword) {
  // Arrange
  PurchaseIntentClassifier purchase_intent_classifier;
  purchase_intent_classifier.LoadUserModelForLocale("en-US");

  const std::string url = "https://duckduckgo.com/?q=segment+funnel+keyword+2";
  const std::string last_url = "https://www.foobar.com";

  // Act
  const PurchaseIntentSignalInfo info =
      purchase_intent_classifier.MaybeExtractIntentSignal(url, last_url);

  // Assert
  const PurchaseIntentSegmentList expected_segments({
    "segment 1",
    "segment 2"
  });

  EXPECT_EQ(expected_segments, info.segments);
  EXPECT_EQ(3, info.weight);
}

}  // namespace behavioral
}  // namespace ad_targeting
}  // namespace ads
