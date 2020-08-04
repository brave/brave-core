/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/classification/purchase_intent_classifier/purchase_intent_classifier.h"

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "testing/gtest/include/gtest/gtest.h"
#include "bat/ads/internal/ads_client_mock.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/time_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace classification {

class BatAdsPurchaseIntentClassifierTest : public ::testing::Test {
 protected:
  BatAdsPurchaseIntentClassifierTest() :
      ads_client_mock_(std::make_unique<AdsClientMock>()),
      ads_(std::make_unique<AdsImpl>(ads_client_mock_.get())),
      purchase_intent_classifier_(std::make_unique<
          PurchaseIntentClassifier>(ads_.get())) {
    // You can do set-up work for each test here
  }

  ~BatAdsPurchaseIntentClassifierTest() override {
    // You can do clean-up work that doesn't throw exceptions here
  }

  // If the constructor and destructor are not enough for setting up and
  // cleaning up each test, you can use the following methods

  void SetUp() override {
    // Code here will be called immediately after the constructor (right before
    // each test)

    const char json[] = R"(
        {
          "locale": "gb",
          "version": 1,
          "timestamp": "2020-05-15 00:00:00",
          "parameters": {
            "signal_level": 1,
            "classification_threshold": 10,
            "signal_decay_time_window_in_seconds": 100
          },
          "segments": [
            "segment 1", "segment 2", "segment 3"
          ],
          "segment_keywords": {
            "segment keyword 1": [0],
            "segment keyword 2": [0, 1]
          },
          "funnel_keywords": {
            "funnel keyword 1": 2,
            "funnel keyword 2": 3
          },
          "funnel_sites": [
            {
              "sites": [
                "http://brave.com", "http://crave.com"
              ],
              "segments": [1, 2]
            },
            {
              "sites": [
                "http://frexample.org", "http://example.org"
              ],
              "segments": [0]
            }
          ]
        })";

    purchase_intent_classifier_->Initialize(json);
  }

  void TearDown() override {
    // Code here will be called immediately after each test (right before the
    // destructor)
  }

  // Objects declared here can be used by all tests in the test case

  std::unique_ptr<AdsClientMock> ads_client_mock_;
  std::unique_ptr<AdsImpl> ads_;
  std::unique_ptr<PurchaseIntentClassifier> purchase_intent_classifier_;
};

TEST_F(BatAdsPurchaseIntentClassifierTest,
    InitializeClassifier) {
  EXPECT_TRUE(purchase_intent_classifier_->IsInitialized());
}

TEST_F(BatAdsPurchaseIntentClassifierTest,
    ExtractSignalAndMatchFunnelSite) {
  // Arrange
  const std::string url = "https://www.brave.com/test?foo=bar";

  // Act
  const PurchaseIntentSignalInfo info =
      purchase_intent_classifier_->MaybeExtractIntentSignal(url);

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
  const std::string url = "https://duckduckgo.com/?q=segment+keyword+1&foo=bar";

  // Act
  const PurchaseIntentSignalInfo info =
      purchase_intent_classifier_->MaybeExtractIntentSignal(url);

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
  const std::string url = "https://duckduckgo.com/?q=segment+funnel+keyword+2";

  // Act
  const PurchaseIntentSignalInfo info =
      purchase_intent_classifier_->MaybeExtractIntentSignal(url);

  // Assert
  const PurchaseIntentSegmentList expected_segments({
    "segment 1",
    "segment 2"
  });

  EXPECT_EQ(expected_segments, info.segments);
  EXPECT_EQ(3, info.weight);
}

}  // namespace classification
}  // namespace ads
