/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/processors/behavioral/purchase_intent/purchase_intent_processor.h"

#include <cstdint>

#include "base/ranges/algorithm.h"
#include "brave/components/brave_ads/core/internal/ads/serving/targeting/behavioral/purchase_intent/purchase_intent_model.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/purchase_intent/purchase_intent_resource.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/purchase_intent/purchase_intent_signal_history_info.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsPurchaseIntentProcessorTest : public UnitTestBase {};

TEST_F(BraveAdsPurchaseIntentProcessorTest,
       DoNotProcessIfResourceIsNotInitialized) {
  // Arrange
  resource::PurchaseIntent resource;

  // Act
  const GURL url = GURL("https://www.brave.com/test?foo=bar");
  processor::PurchaseIntent processor(resource);
  processor.Process(url);

  // Assert
  const targeting::PurchaseIntentSignalHistoryMap& history =
      ClientStateManager::GetInstance().GetPurchaseIntentSignalHistory();

  EXPECT_TRUE(history.empty());
}

TEST_F(BraveAdsPurchaseIntentProcessorTest, DoNotProcessForInvalidUrl) {
  // Arrange
  resource::PurchaseIntent resource;
  resource.Load();
  task_environment_.RunUntilIdle();

  // Act
  const GURL url = GURL("invalid_url");
  processor::PurchaseIntent processor(resource);
  processor.Process(url);

  // Assert
  const targeting::PurchaseIntentSignalHistoryMap& history =
      ClientStateManager::GetInstance().GetPurchaseIntentSignalHistory();

  EXPECT_TRUE(history.empty());
}

TEST_F(BraveAdsPurchaseIntentProcessorTest, NeverProcessed) {
  // Arrange
  resource::PurchaseIntent resource;
  resource.Load();
  task_environment_.RunUntilIdle();

  // Act
  const targeting::model::PurchaseIntent model;
  const SegmentList segments = model.GetSegments();

  // Assert
  const targeting::PurchaseIntentSignalHistoryMap history =
      ClientStateManager::GetInstance().GetPurchaseIntentSignalHistory();

  EXPECT_TRUE(history.empty());
}

TEST_F(BraveAdsPurchaseIntentProcessorTest, ProcessUrl) {
  // Arrange
  resource::PurchaseIntent resource;
  resource.Load();
  task_environment_.RunUntilIdle();

  // Act
  const GURL url = GURL("https://www.brave.com/test?foo=bar");
  processor::PurchaseIntent processor(resource);
  processor.Process(url);

  // Assert
  const targeting::PurchaseIntentSignalHistoryMap& history =
      ClientStateManager::GetInstance().GetPurchaseIntentSignalHistory();

  const base::Time now = Now();
  const uint16_t weight = 1;

  const targeting::PurchaseIntentSignalHistoryMap expected_history = {
      {"segment 2", {targeting::PurchaseIntentSignalHistoryInfo(now, weight)}},
      {"segment 3", {targeting::PurchaseIntentSignalHistoryInfo(now, weight)}}};

  EXPECT_TRUE(base::ranges::equal(expected_history, history));
}

TEST_F(BraveAdsPurchaseIntentProcessorTest, ProcessMultipleMatchingUrls) {
  // Arrange
  resource::PurchaseIntent resource;
  resource.Load();
  task_environment_.RunUntilIdle();

  // Act
  processor::PurchaseIntent processor(resource);

  const GURL url = GURL("https://www.brave.com/test?foo=bar");
  processor.Process(url);
  processor.Process(url);

  // Assert
  const targeting::PurchaseIntentSignalHistoryMap& history =
      ClientStateManager::GetInstance().GetPurchaseIntentSignalHistory();

  const base::Time now = Now();
  const uint16_t weight = 1;

  const targeting::PurchaseIntentSignalHistoryMap expected_history = {
      {"segment 2",
       {targeting::PurchaseIntentSignalHistoryInfo(now, weight),
        targeting::PurchaseIntentSignalHistoryInfo(now, weight)}},
      {"segment 3",
       {targeting::PurchaseIntentSignalHistoryInfo(now, weight),
        targeting::PurchaseIntentSignalHistoryInfo(now, weight)}}};

  EXPECT_TRUE(base::ranges::equal(expected_history, history));
}

TEST_F(BraveAdsPurchaseIntentProcessorTest, ProcessMultipleUniqueUrls) {
  // Arrange
  resource::PurchaseIntent resource;
  resource.Load();
  task_environment_.RunUntilIdle();

  // Act
  processor::PurchaseIntent processor(resource);

  const base::Time now_1 = Now();
  const GURL url_1 = GURL("https://www.brave.com/test?foo=bar");
  processor.Process(url_1);

  AdvanceClockBy(base::Minutes(5));

  const base::Time now_2 = Now();
  const GURL url_2 = GURL("https://www.basicattentiontoken.org/test?foo=bar");
  processor.Process(url_2);

  // Assert
  const targeting::PurchaseIntentSignalHistoryMap& history =
      ClientStateManager::GetInstance().GetPurchaseIntentSignalHistory();

  const uint16_t weight = 1;

  const targeting::PurchaseIntentSignalHistoryMap expected_history = {
      {"segment 2",
       {targeting::PurchaseIntentSignalHistoryInfo(now_1, weight),
        targeting::PurchaseIntentSignalHistoryInfo(now_2, weight)}},
      {"segment 3",
       {targeting::PurchaseIntentSignalHistoryInfo(now_1, weight),
        targeting::PurchaseIntentSignalHistoryInfo(now_2, weight)}}};

  EXPECT_TRUE(base::ranges::equal(expected_history, history));
}

TEST_F(BraveAdsPurchaseIntentProcessorTest, ProcessMultipleMatchingKeywords) {
  // Arrange
  resource::PurchaseIntent resource;
  resource.Load();
  task_environment_.RunUntilIdle();

  // Act
  processor::PurchaseIntent processor(resource);

  const base::Time now_1 = Now();
  const GURL url_1 =
      GURL("https://duckduckgo.com/?q=segment+keyword+1&foo=bar");
  processor.Process(url_1);

  AdvanceClockBy(base::Minutes(5));

  const base::Time now_2 = Now();
  const GURL url_2 =
      GURL("https://duckduckgo.com/?q=segment+keyword+2&bar=foo");
  processor.Process(url_2);

  // Assert
  const targeting::PurchaseIntentSignalHistoryMap& history =
      ClientStateManager::GetInstance().GetPurchaseIntentSignalHistory();

  const uint16_t weight = 1;

  const targeting::PurchaseIntentSignalHistoryMap expected_history = {
      {"segment 1",
       {targeting::PurchaseIntentSignalHistoryInfo(now_1, weight),
        targeting::PurchaseIntentSignalHistoryInfo(now_2, weight)}},
      {"segment 2",
       {targeting::PurchaseIntentSignalHistoryInfo(now_2, weight)}}};

  EXPECT_TRUE(base::ranges::equal(expected_history, history));
}

TEST_F(BraveAdsPurchaseIntentProcessorTest, ProcessMultipleUniqueKeywords) {
  // Arrange
  resource::PurchaseIntent resource;
  resource.Load();
  task_environment_.RunUntilIdle();

  // Act
  processor::PurchaseIntent processor(resource);

  const base::Time now_1 = Now();
  const GURL url_1 =
      GURL("https://duckduckgo.com/?q=segment+keyword+1&foo=bar");
  processor.Process(url_1);

  AdvanceClockBy(base::Minutes(5));

  const base::Time now_2 = Now();
  const GURL url_2 =
      GURL("https://www.google.com/search?q=segment+keyword+1&bar=foo");
  processor.Process(url_2);

  // Assert
  const targeting::PurchaseIntentSignalHistoryMap& history =
      ClientStateManager::GetInstance().GetPurchaseIntentSignalHistory();

  const uint16_t weight = 1;

  const targeting::PurchaseIntentSignalHistoryMap expected_history = {
      {"segment 1",
       {targeting::PurchaseIntentSignalHistoryInfo(now_1, weight),
        targeting::PurchaseIntentSignalHistoryInfo(now_2, weight)}}};

  EXPECT_TRUE(base::ranges::equal(expected_history, history));
}

TEST_F(BraveAdsPurchaseIntentProcessorTest, ProcessSegmentAndFunnelKeywords) {
  // Arrange
  resource::PurchaseIntent resource;
  resource.Load();
  task_environment_.RunUntilIdle();

  // Act
  processor::PurchaseIntent processor(resource);

  const GURL url =
      GURL("https://duckduckgo.com/?q=segment+keyword+1+funnel+keyword+2");
  processor.Process(url);

  // Assert
  const targeting::PurchaseIntentSignalHistoryMap& history =
      ClientStateManager::GetInstance().GetPurchaseIntentSignalHistory();

  const base::Time now = Now();
  const uint16_t weight = 3;

  const targeting::PurchaseIntentSignalHistoryMap expected_history = {
      {"segment 1",
       {
           targeting::PurchaseIntentSignalHistoryInfo(now, weight),
       }}};

  EXPECT_TRUE(base::ranges::equal(expected_history, history));
}

}  // namespace brave_ads
