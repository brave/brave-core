/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/purchase_intent_processor.h"

#include <memory>

#include "brave/components/brave_ads/core/internal/common/resources/country_components_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_resource.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_signal_history_info.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsPurchaseIntentProcessorTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    resource_ = std::make_unique<PurchaseIntentResource>();
  }

  bool LoadResource() {
    NotifyDidUpdateResourceComponent(kCountryComponentManifestVersion,
                                     kCountryComponentId);
    task_environment_.RunUntilIdle();
    return resource_->IsInitialized();
  }

  std::unique_ptr<PurchaseIntentResource> resource_;
};

TEST_F(BraveAdsPurchaseIntentProcessorTest,
       DoNotProcessIfResourceIsNotInitialized) {
  // Arrange
  PurchaseIntentProcessor processor(*resource_);

  // Act
  processor.Process(GURL("https://www.brave.com/test?foo=bar"));

  // Assert
  const PurchaseIntentSignalHistoryMap& history =
      ClientStateManager::GetInstance().GetPurchaseIntentSignalHistory();
  EXPECT_TRUE(history.empty());
}

TEST_F(BraveAdsPurchaseIntentProcessorTest, DoNotProcessForInvalidUrl) {
  // Arrange
  ASSERT_TRUE(LoadResource());

  PurchaseIntentProcessor processor(*resource_);

  // Act
  processor.Process(GURL("INVALID"));

  // Assert
  const PurchaseIntentSignalHistoryMap& history =
      ClientStateManager::GetInstance().GetPurchaseIntentSignalHistory();
  EXPECT_TRUE(history.empty());
}

TEST_F(BraveAdsPurchaseIntentProcessorTest, NeverProcessed) {
  // Arrange
  ASSERT_TRUE(LoadResource());

  // Assert
  const PurchaseIntentSignalHistoryMap history =
      ClientStateManager::GetInstance().GetPurchaseIntentSignalHistory();
  EXPECT_TRUE(history.empty());
}

TEST_F(BraveAdsPurchaseIntentProcessorTest, ProcessUrl) {
  // Arrange
  ASSERT_TRUE(LoadResource());

  PurchaseIntentProcessor processor(*resource_);

  // Act
  processor.Process(GURL("https://www.brave.com/test?foo=bar"));

  // Assert
  const PurchaseIntentSignalHistoryMap& history =
      ClientStateManager::GetInstance().GetPurchaseIntentSignalHistory();

  const PurchaseIntentSignalHistoryMap expected_history = {
      {"segment 2", {PurchaseIntentSignalHistoryInfo(Now(), /*weight=*/1)}},
      {"segment 3", {PurchaseIntentSignalHistoryInfo(Now(), /*weight=*/1)}}};
  EXPECT_THAT(expected_history, ::testing::ElementsAreArray(history));
}

TEST_F(BraveAdsPurchaseIntentProcessorTest, ProcessMultipleMatchingUrls) {
  // Arrange
  ASSERT_TRUE(LoadResource());

  const GURL url = GURL("https://www.brave.com/test?foo=bar");

  PurchaseIntentProcessor processor(*resource_);

  // Act
  processor.Process(url);
  processor.Process(url);

  // Assert
  const PurchaseIntentSignalHistoryMap& history =
      ClientStateManager::GetInstance().GetPurchaseIntentSignalHistory();

  const PurchaseIntentSignalHistoryMap expected_history = {
      {"segment 2",
       {PurchaseIntentSignalHistoryInfo(Now(), /*weight=*/1),
        PurchaseIntentSignalHistoryInfo(Now(), /*weight=*/1)}},
      {"segment 3",
       {PurchaseIntentSignalHistoryInfo(Now(), /*weight=*/1),
        PurchaseIntentSignalHistoryInfo(Now(), /*weight=*/1)}}};
  EXPECT_THAT(expected_history, ::testing::ElementsAreArray(history));
}

TEST_F(BraveAdsPurchaseIntentProcessorTest, ProcessMultipleUniqueUrls) {
  // Arrange
  ASSERT_TRUE(LoadResource());

  const base::Time now_before_advancing_clock = Now();

  PurchaseIntentProcessor processor(*resource_);
  processor.Process(GURL("https://www.brave.com/test?foo=bar"));

  AdvanceClockBy(base::Minutes(5));

  // Act
  processor.Process(GURL("https://www.basicattentiontoken.org/test?foo=bar"));

  // Assert
  const PurchaseIntentSignalHistoryMap& history =
      ClientStateManager::GetInstance().GetPurchaseIntentSignalHistory();

  const PurchaseIntentSignalHistoryMap expected_history = {
      {"segment 2",
       {PurchaseIntentSignalHistoryInfo(now_before_advancing_clock,
                                        /*weight=*/1),
        PurchaseIntentSignalHistoryInfo(Now(), /*weight=*/1)}},
      {"segment 3",
       {PurchaseIntentSignalHistoryInfo(now_before_advancing_clock,
                                        /*weight=*/1),
        PurchaseIntentSignalHistoryInfo(Now(), /*weight=*/1)}}};
  EXPECT_THAT(expected_history, ::testing::ElementsAreArray(history));
}

TEST_F(BraveAdsPurchaseIntentProcessorTest, ProcessMultipleMatchingKeywords) {
  // Arrange
  ASSERT_TRUE(LoadResource());

  PurchaseIntentProcessor processor(*resource_);
  processor.Process(
      GURL("https://duckduckgo.com/?q=segment+keyword+1&foo=bar"));

  const base::Time now_before_advancing_clock = Now();

  AdvanceClockBy(base::Minutes(5));

  // Act
  processor.Process(
      GURL("https://duckduckgo.com/?q=segment+keyword+2&bar=foo"));

  // Assert
  const PurchaseIntentSignalHistoryMap& history =
      ClientStateManager::GetInstance().GetPurchaseIntentSignalHistory();

  const PurchaseIntentSignalHistoryMap expected_history = {
      {"segment 1",
       {PurchaseIntentSignalHistoryInfo(now_before_advancing_clock,
                                        /*weight=*/1),
        PurchaseIntentSignalHistoryInfo(Now(), /*weight=*/1)}},
      {"segment 2", {PurchaseIntentSignalHistoryInfo(Now(), /*weight=*/1)}}};
  EXPECT_THAT(expected_history, ::testing::ElementsAreArray(history));
}

TEST_F(BraveAdsPurchaseIntentProcessorTest, ProcessMultipleUniqueKeywords) {
  // Arrange
  ASSERT_TRUE(LoadResource());

  PurchaseIntentProcessor processor(*resource_);
  processor.Process(
      GURL("https://duckduckgo.com/?q=segment+keyword+1&foo=bar"));

  const base::Time now_before_advancing_clock = Now();

  AdvanceClockBy(base::Minutes(5));

  // Act
  processor.Process(
      GURL("https://www.google.com/search?q=segment+keyword+1&bar=foo"));

  // Assert
  const PurchaseIntentSignalHistoryMap expected_history = {
      {"segment 1",
       {PurchaseIntentSignalHistoryInfo(now_before_advancing_clock,
                                        /*weight=*/1),
        PurchaseIntentSignalHistoryInfo(Now(), /*weight=*/1)}}};
  EXPECT_EQ(expected_history,
            ClientStateManager::GetInstance().GetPurchaseIntentSignalHistory());
}

TEST_F(BraveAdsPurchaseIntentProcessorTest, ProcessSegmentAndFunnelKeywords) {
  // Arrange
  ASSERT_TRUE(LoadResource());

  PurchaseIntentProcessor processor(*resource_);

  // Act
  processor.Process(
      GURL("https://duckduckgo.com/?q=segment+keyword+1+funnel+keyword+2"));

  // Assert
  const PurchaseIntentSignalHistoryMap expected_history = {
      {"segment 1",
       {
           PurchaseIntentSignalHistoryInfo(Now(), /*weight=*/3),
       }}};
  EXPECT_EQ(expected_history,
            ClientStateManager::GetInstance().GetPurchaseIntentSignalHistory());
}

}  // namespace brave_ads
