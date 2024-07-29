/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/purchase_intent_processor.h"

#include <memory>

#include "brave/components/brave_ads/core/internal/common/resources/country_components_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_resource.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_signal_history_info.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsPurchaseIntentProcessorTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    resource_ = std::make_unique<PurchaseIntentResource>();
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
  const PurchaseIntentSignalHistoryMap& purchase_intent_signal_history =
      ClientStateManager::GetInstance().GetPurchaseIntentSignalHistory();
  EXPECT_THAT(purchase_intent_signal_history, ::testing::IsEmpty());
}

TEST_F(BraveAdsPurchaseIntentProcessorTest,
       DoNotProcessForUnsupportedUrlScheme) {
  // Arrange
  NotifyResourceComponentDidChange(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);
  ASSERT_TRUE(resource_->IsLoaded());

  PurchaseIntentProcessor processor(*resource_);

  // Act
  processor.Process(GURL("brave://rewards"));

  // Assert
  const PurchaseIntentSignalHistoryMap& purchase_intent_signal_history =
      ClientStateManager::GetInstance().GetPurchaseIntentSignalHistory();
  EXPECT_THAT(purchase_intent_signal_history, ::testing::IsEmpty());
}

TEST_F(BraveAdsPurchaseIntentProcessorTest, DoNotProcessForInvalidUrl) {
  // Arrange
  NotifyResourceComponentDidChange(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);
  ASSERT_TRUE(resource_->IsLoaded());

  PurchaseIntentProcessor processor(*resource_);

  // Act
  processor.Process(GURL("INVALID"));

  // Assert
  const PurchaseIntentSignalHistoryMap& purchase_intent_signal_history =
      ClientStateManager::GetInstance().GetPurchaseIntentSignalHistory();
  EXPECT_THAT(purchase_intent_signal_history, ::testing::IsEmpty());
}

TEST_F(BraveAdsPurchaseIntentProcessorTest, NeverProcessed) {
  // Arrange
  NotifyResourceComponentDidChange(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);
  ASSERT_TRUE(resource_->IsLoaded());

  // Act & Assert
  const PurchaseIntentSignalHistoryMap& purchase_intent_signal_history =
      ClientStateManager::GetInstance().GetPurchaseIntentSignalHistory();
  EXPECT_THAT(purchase_intent_signal_history, ::testing::IsEmpty());
}

TEST_F(BraveAdsPurchaseIntentProcessorTest, ProcessSignalForUrl) {
  // Arrange
  NotifyResourceComponentDidChange(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);
  ASSERT_TRUE(resource_->IsLoaded());

  PurchaseIntentProcessor processor(*resource_);

  // Act
  processor.Process(GURL("https://www.brave.com/test?foo=bar"));

  // Assert
  const PurchaseIntentSignalHistoryMap expected_purchase_intent_signal_history =
      {{"segment 2",
        {PurchaseIntentSignalHistoryInfo(/*at=*/test::Now(), /*weight=*/1)}},
       {"segment 3",
        {PurchaseIntentSignalHistoryInfo(/*at=*/test::Now(), /*weight=*/1)}}};

  const PurchaseIntentSignalHistoryMap& purchase_intent_signal_history =
      ClientStateManager::GetInstance().GetPurchaseIntentSignalHistory();

  EXPECT_THAT(expected_purchase_intent_signal_history,
              ::testing::ElementsAreArray(purchase_intent_signal_history));
}

TEST_F(BraveAdsPurchaseIntentProcessorTest,
       ProcessMultipleSignalsForMatchingUrls) {
  // Arrange
  NotifyResourceComponentDidChange(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);
  ASSERT_TRUE(resource_->IsLoaded());

  const GURL url = GURL("https://www.brave.com/test?foo=bar");

  PurchaseIntentProcessor processor(*resource_);

  // Act
  processor.Process(url);
  processor.Process(url);

  // Assert
  const PurchaseIntentSignalHistoryMap expected_purchase_intent_signal_history =
      {{"segment 2",
        {PurchaseIntentSignalHistoryInfo(/*at=*/test::Now(), /*weight=*/1),
         PurchaseIntentSignalHistoryInfo(/*at=*/test::Now(), /*weight=*/1)}},
       {"segment 3",
        {PurchaseIntentSignalHistoryInfo(/*at=*/test::Now(), /*weight=*/1),
         PurchaseIntentSignalHistoryInfo(/*at=*/test::Now(), /*weight=*/1)}}};

  const PurchaseIntentSignalHistoryMap& purchase_intent_signal_history =
      ClientStateManager::GetInstance().GetPurchaseIntentSignalHistory();

  EXPECT_THAT(expected_purchase_intent_signal_history,
              ::testing::ElementsAreArray(purchase_intent_signal_history));
}

TEST_F(BraveAdsPurchaseIntentProcessorTest,
       ProcessMultipleSignalsForUniqueUrls) {
  // Arrange
  NotifyResourceComponentDidChange(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);
  ASSERT_TRUE(resource_->IsLoaded());

  const base::Time at_before_advancing_clock = test::Now();

  PurchaseIntentProcessor processor(*resource_);
  processor.Process(GURL("https://www.brave.com/test?foo=bar"));

  AdvanceClockBy(base::Minutes(5));

  // Act
  processor.Process(GURL("https://basicattentiontoken.org/test?foo=bar"));

  // Assert
  const PurchaseIntentSignalHistoryMap expected_purchase_intent_signal_history =
      {{"segment 2",
        {PurchaseIntentSignalHistoryInfo(at_before_advancing_clock,
                                         /*weight=*/1),
         PurchaseIntentSignalHistoryInfo(/*at=*/test::Now(), /*weight=*/1)}},
       {"segment 3",
        {PurchaseIntentSignalHistoryInfo(at_before_advancing_clock,
                                         /*weight=*/1),
         PurchaseIntentSignalHistoryInfo(/*at=*/test::Now(), /*weight=*/1)}}};

  const PurchaseIntentSignalHistoryMap& purchase_intent_signal_history =
      ClientStateManager::GetInstance().GetPurchaseIntentSignalHistory();

  EXPECT_THAT(expected_purchase_intent_signal_history,
              ::testing::ElementsAreArray(purchase_intent_signal_history));
}

TEST_F(BraveAdsPurchaseIntentProcessorTest,
       ProcessMultipleSearchQuerySignalsForMatchingKeywords) {
  // Arrange
  NotifyResourceComponentDidChange(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);
  ASSERT_TRUE(resource_->IsLoaded());

  PurchaseIntentProcessor processor(*resource_);
  processor.Process(
      GURL("https://duckduckgo.com/?q=segment+keyword+1&foo=bar"));

  const base::Time signaled_at_before_advancing_clock = test::Now();

  AdvanceClockBy(base::Minutes(5));

  // Act
  processor.Process(
      GURL("https://duckduckgo.com/?q=segment+keyword+2&bar=foo"));

  // Assert
  const PurchaseIntentSignalHistoryMap expected_purchase_intent_signal_history =
      {{"segment 1",
        {PurchaseIntentSignalHistoryInfo(signaled_at_before_advancing_clock,
                                         /*weight=*/1),
         PurchaseIntentSignalHistoryInfo(/*at=*/test::Now(), /*weight=*/1)}},
       {"segment 2",
        {PurchaseIntentSignalHistoryInfo(/*at=*/test::Now(), /*weight=*/1)}}};

  const PurchaseIntentSignalHistoryMap& purchase_intent_signal_history =
      ClientStateManager::GetInstance().GetPurchaseIntentSignalHistory();

  EXPECT_THAT(expected_purchase_intent_signal_history,
              ::testing::ElementsAreArray(purchase_intent_signal_history));
}

TEST_F(BraveAdsPurchaseIntentProcessorTest,
       ProcessMultipleSearchQuerySignalsForUniqueKeywords) {
  // Arrange
  NotifyResourceComponentDidChange(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);
  ASSERT_TRUE(resource_->IsLoaded());

  PurchaseIntentProcessor processor(*resource_);
  processor.Process(
      GURL("https://duckduckgo.com/?q=segment+keyword+1&foo=bar"));

  const base::Time signaled_at_before_advancing_clock = test::Now();

  AdvanceClockBy(base::Minutes(5));

  // Act
  processor.Process(
      GURL("https://www.google.com/search?q=segment+keyword+1&bar=foo"));

  // Assert
  const PurchaseIntentSignalHistoryMap expected_purchase_intent_signal_history =
      {{"segment 1",
        {PurchaseIntentSignalHistoryInfo(signaled_at_before_advancing_clock,
                                         /*weight=*/1),
         PurchaseIntentSignalHistoryInfo(/*at=*/test::Now(), /*weight=*/1)}}};

  const PurchaseIntentSignalHistoryMap& purchase_intent_signal_history =
      ClientStateManager::GetInstance().GetPurchaseIntentSignalHistory();

  EXPECT_EQ(expected_purchase_intent_signal_history,
            purchase_intent_signal_history);
}

TEST_F(BraveAdsPurchaseIntentProcessorTest,
       ProcessSearchQuerySignalForSegmentAndFunnelKeywords) {
  // Arrange
  NotifyResourceComponentDidChange(test::kCountryComponentManifestVersion,
                                   test::kCountryComponentId);
  ASSERT_TRUE(resource_->IsLoaded());

  PurchaseIntentProcessor processor(*resource_);

  // Act
  processor.Process(
      GURL("https://duckduckgo.com/?q=segment+keyword+1+funnel+keyword+2"));

  // Assert
  const PurchaseIntentSignalHistoryMap expected_purchase_intent_signal_history =
      {{"segment 1",
        {
            PurchaseIntentSignalHistoryInfo(/*at=*/test::Now(), /*weight=*/3),
        }}};

  const PurchaseIntentSignalHistoryMap& purchase_intent_signal_history =
      ClientStateManager::GetInstance().GetPurchaseIntentSignalHistory();

  EXPECT_EQ(expected_purchase_intent_signal_history,
            purchase_intent_signal_history);
}

}  // namespace brave_ads
