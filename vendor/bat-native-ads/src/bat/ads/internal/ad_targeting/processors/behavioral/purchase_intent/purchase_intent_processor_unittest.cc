/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_targeting/processors/behavioral/purchase_intent/purchase_intent_processor.h"

#include "bat/ads/internal/ad_serving/ad_targeting/models/behavioral/purchase_intent/purchase_intent_model.h"
#include "bat/ads/internal/container_util.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace ad_targeting {

class BatAdsPurchaseIntentProcessorTest : public UnitTestBase {
 protected:
  BatAdsPurchaseIntentProcessorTest() = default;

  ~BatAdsPurchaseIntentProcessorTest() override = default;
};

TEST_F(BatAdsPurchaseIntentProcessorTest,
       DoNotProcessIfResourceIsNotInitialized) {
  // Arrange
  resource::PurchaseIntent resource;

  // Act
  const GURL url = GURL("https://www.brave.com/test?foo=bar");
  processor::PurchaseIntent processor(&resource);
  processor.Process(url);

  // Assert
  const PurchaseIntentSignalHistoryMap history =
      Client::Get()->GetPurchaseIntentSignalHistory();

  EXPECT_TRUE(history.empty());
}

TEST_F(BatAdsPurchaseIntentProcessorTest, DoNotProcessForInvalidUrl) {
  // Arrange
  resource::PurchaseIntent resource;
  resource.Load();

  // Act
  const GURL url = GURL("invalid_url");
  processor::PurchaseIntent processor(&resource);
  processor.Process(url);

  // Assert
  const PurchaseIntentSignalHistoryMap history =
      Client::Get()->GetPurchaseIntentSignalHistory();

  EXPECT_TRUE(history.empty());
}

TEST_F(BatAdsPurchaseIntentProcessorTest, NeverProcessed) {
  // Arrange
  resource::PurchaseIntent resource;
  resource.Load();

  // Act
  model::PurchaseIntent model;
  const SegmentList segments = model.GetSegments();

  // Assert
  const PurchaseIntentSignalHistoryMap history =
      Client::Get()->GetPurchaseIntentSignalHistory();

  EXPECT_TRUE(history.empty());
}

TEST_F(BatAdsPurchaseIntentProcessorTest, ProcessUrl) {
  // Arrange
  resource::PurchaseIntent resource;
  resource.Load();

  // Act
  const GURL url = GURL("https://www.brave.com/test?foo=bar");
  processor::PurchaseIntent processor(&resource);
  processor.Process(url);

  // Assert
  const PurchaseIntentSignalHistoryMap history =
      Client::Get()->GetPurchaseIntentSignalHistory();

  const int64_t now = NowAsTimestamp();
  const uint16_t weight = 1;

  const PurchaseIntentSignalHistoryMap expected_history = {
      {"segment 2", {PurchaseIntentSignalHistoryInfo(now, weight)}},
      {"segment 3", {PurchaseIntentSignalHistoryInfo(now, weight)}}};

  EXPECT_TRUE(CompareMaps(expected_history, history));
}

TEST_F(BatAdsPurchaseIntentProcessorTest, ProcessMultipleMatchingUrls) {
  // Arrange
  resource::PurchaseIntent resource;
  resource.Load();

  // Act
  processor::PurchaseIntent processor(&resource);

  const GURL url = GURL("https://www.brave.com/test?foo=bar");
  processor.Process(url);
  processor.Process(url);

  // Assert
  const PurchaseIntentSignalHistoryMap history =
      Client::Get()->GetPurchaseIntentSignalHistory();

  const int64_t now = NowAsTimestamp();
  const uint16_t weight = 1;

  const PurchaseIntentSignalHistoryMap expected_history = {
      {"segment 2",
       {PurchaseIntentSignalHistoryInfo(now, weight),
        PurchaseIntentSignalHistoryInfo(now, weight)}},
      {"segment 3",
       {PurchaseIntentSignalHistoryInfo(now, weight),
        PurchaseIntentSignalHistoryInfo(now, weight)}}};

  EXPECT_TRUE(CompareMaps(expected_history, history));
}

TEST_F(BatAdsPurchaseIntentProcessorTest, ProcessMultipleUniqueUrls) {
  // Arrange
  resource::PurchaseIntent resource;
  resource.Load();

  // Act
  processor::PurchaseIntent processor(&resource);

  const int64_t now_1 = NowAsTimestamp();
  const GURL url_1 = GURL("https://www.brave.com/test?foo=bar");
  processor.Process(url_1);

  FastForwardClockBy(base::TimeDelta::FromMinutes(5));

  const int64_t now_2 = NowAsTimestamp();
  const GURL url_2 = GURL("https://www.basicattentiontoken.org/test?foo=bar");
  processor.Process(url_2);

  // Assert
  const PurchaseIntentSignalHistoryMap history =
      Client::Get()->GetPurchaseIntentSignalHistory();

  const uint16_t weight = 1;

  const PurchaseIntentSignalHistoryMap expected_history = {
      {"segment 2",
       {PurchaseIntentSignalHistoryInfo(now_1, weight),
        PurchaseIntentSignalHistoryInfo(now_2, weight)}},
      {"segment 3",
       {PurchaseIntentSignalHistoryInfo(now_1, weight),
        PurchaseIntentSignalHistoryInfo(now_2, weight)}}};

  EXPECT_TRUE(CompareMaps(expected_history, history));
}

TEST_F(BatAdsPurchaseIntentProcessorTest, ProcessMultipleMatchingKeywords) {
  // Arrange
  resource::PurchaseIntent resource;
  resource.Load();

  // Act
  processor::PurchaseIntent processor(&resource);

  const int64_t now_1 = NowAsTimestamp();
  const GURL url_1 =
      GURL("https://duckduckgo.com/?q=segment+keyword+1&foo=bar");
  processor.Process(url_1);

  FastForwardClockBy(base::TimeDelta::FromMinutes(5));

  const int64_t now_2 = NowAsTimestamp();
  const GURL url_2 =
      GURL("https://duckduckgo.com/?q=segment+keyword+2&bar=foo");
  processor.Process(url_2);

  // Assert
  const PurchaseIntentSignalHistoryMap history =
      Client::Get()->GetPurchaseIntentSignalHistory();

  const uint16_t weight = 1;

  const PurchaseIntentSignalHistoryMap expected_history = {
      {"segment 1",
       {PurchaseIntentSignalHistoryInfo(now_1, weight),
        PurchaseIntentSignalHistoryInfo(now_2, weight)}},
      {"segment 2", {PurchaseIntentSignalHistoryInfo(now_2, weight)}}};

  EXPECT_TRUE(CompareMaps(expected_history, history));
}

TEST_F(BatAdsPurchaseIntentProcessorTest, ProcessMultipleUniqueKeywords) {
  // Arrange
  resource::PurchaseIntent resource;
  resource.Load();

  // Act
  processor::PurchaseIntent processor(&resource);

  const int64_t now_1 = NowAsTimestamp();
  const GURL url_1 =
      GURL("https://duckduckgo.com/?q=segment+keyword+1&foo=bar");
  processor.Process(url_1);

  FastForwardClockBy(base::TimeDelta::FromMinutes(5));

  const int64_t now_2 = NowAsTimestamp();
  const GURL url_2 = GURL("https://google.com/?q=segment+keyword+1&bar=foo");
  processor.Process(url_2);

  // Assert
  const PurchaseIntentSignalHistoryMap history =
      Client::Get()->GetPurchaseIntentSignalHistory();

  const uint16_t weight = 1;

  const PurchaseIntentSignalHistoryMap expected_history = {
      {"segment 1",
       {PurchaseIntentSignalHistoryInfo(now_1, weight),
        PurchaseIntentSignalHistoryInfo(now_2, weight)}}};

  EXPECT_TRUE(CompareMaps(expected_history, history));
}

TEST_F(BatAdsPurchaseIntentProcessorTest, ProcessSegmentAndFunnelKeywords) {
  // Arrange
  resource::PurchaseIntent resource;
  resource.Load();

  // Act
  processor::PurchaseIntent processor(&resource);

  const GURL url =
      GURL("https://duckduckgo.com/?q=segment+keyword+1+funnel+keyword+2");
  processor.Process(url);

  // Assert
  const PurchaseIntentSignalHistoryMap history =
      Client::Get()->GetPurchaseIntentSignalHistory();

  const int64_t now = NowAsTimestamp();
  const uint16_t weight = 3;

  const PurchaseIntentSignalHistoryMap expected_history = {
      {"segment 1",
       {
           PurchaseIntentSignalHistoryInfo(now, weight),
       }}};

  EXPECT_TRUE(CompareMaps(expected_history, history));
}

}  // namespace ad_targeting
}  // namespace ads
