/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/targeting/processors/behavioral/purchase_intent/purchase_intent_processor.h"

#include <cstdint>

#include "bat/ads/internal/base/container_util.h"
#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/base/unittest_time_util.h"
#include "bat/ads/internal/base/unittest_util.h"
#include "bat/ads/internal/deprecated/client/client.h"
#include "bat/ads/internal/resources/behavioral/purchase_intent/purchase_intent_resource.h"
#include "bat/ads/internal/serving/targeting/models/behavioral/purchase_intent/purchase_intent_model.h"
#include "bat/ads/internal/targeting/data_types/behavioral/purchase_intent/purchase_intent_aliases.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace targeting {

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
  task_environment()->RunUntilIdle();

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
  task_environment()->RunUntilIdle();

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
  task_environment()->RunUntilIdle();

  // Act
  const GURL url = GURL("https://www.brave.com/test?foo=bar");
  processor::PurchaseIntent processor(&resource);
  processor.Process(url);

  // Assert
  const PurchaseIntentSignalHistoryMap history =
      Client::Get()->GetPurchaseIntentSignalHistory();

  const base::Time now = Now();
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
  task_environment()->RunUntilIdle();

  // Act
  processor::PurchaseIntent processor(&resource);

  const GURL url = GURL("https://www.brave.com/test?foo=bar");
  processor.Process(url);
  processor.Process(url);

  // Assert
  const PurchaseIntentSignalHistoryMap history =
      Client::Get()->GetPurchaseIntentSignalHistory();

  const base::Time now = Now();
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
  task_environment()->RunUntilIdle();

  // Act
  processor::PurchaseIntent processor(&resource);

  const base::Time now_1 = Now();
  const GURL url_1 = GURL("https://www.brave.com/test?foo=bar");
  processor.Process(url_1);

  FastForwardClockBy(base::Minutes(5));

  const base::Time now_2 = Now();
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
  task_environment()->RunUntilIdle();

  // Act
  processor::PurchaseIntent processor(&resource);

  const base::Time now_1 = Now();
  const GURL url_1 =
      GURL("https://duckduckgo.com/?q=segment+keyword+1&foo=bar");
  processor.Process(url_1);

  FastForwardClockBy(base::Minutes(5));

  const base::Time now_2 = Now();
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
  task_environment()->RunUntilIdle();

  // Act
  processor::PurchaseIntent processor(&resource);

  const base::Time now_1 = Now();
  const GURL url_1 =
      GURL("https://duckduckgo.com/?q=segment+keyword+1&foo=bar");
  processor.Process(url_1);

  FastForwardClockBy(base::Minutes(5));

  const base::Time now_2 = Now();
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
  task_environment()->RunUntilIdle();

  // Act
  processor::PurchaseIntent processor(&resource);

  const GURL url =
      GURL("https://duckduckgo.com/?q=segment+keyword+1+funnel+keyword+2");
  processor.Process(url);

  // Assert
  const PurchaseIntentSignalHistoryMap history =
      Client::Get()->GetPurchaseIntentSignalHistory();

  const base::Time now = Now();
  const uint16_t weight = 3;

  const PurchaseIntentSignalHistoryMap expected_history = {
      {"segment 1",
       {
           PurchaseIntentSignalHistoryInfo(now, weight),
       }}};

  EXPECT_TRUE(CompareMaps(expected_history, history));
}

}  // namespace targeting
}  // namespace ads
