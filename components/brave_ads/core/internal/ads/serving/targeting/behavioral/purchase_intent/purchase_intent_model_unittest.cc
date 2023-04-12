/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/targeting/behavioral/purchase_intent/purchase_intent_model.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/processors/behavioral/purchase_intent/purchase_intent_processor.h"
#include "brave/components/brave_ads/core/internal/resources/behavioral/purchase_intent/purchase_intent_resource.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads::targeting::model {

class BatAdsPurchaseIntentModelTest : public UnitTestBase {};

TEST_F(BatAdsPurchaseIntentModelTest, DoNotGetSegmentsForUnitializedResource) {
  // Arrange
  resource::PurchaseIntent resource;
  processor::PurchaseIntent processor(&resource);

  const GURL url = GURL("https://www.brave.com/test?foo=bar");
  processor.Process(url);

  const PurchaseIntent model;

  // Act
  const SegmentList segments = model.GetSegments();

  // Assert
  EXPECT_TRUE(segments.empty());
}

TEST_F(BatAdsPurchaseIntentModelTest, DoNotGetSegmentsForExpiredSignals) {
  // Arrange
  resource::PurchaseIntent resource;
  resource.Load();
  task_environment_.RunUntilIdle();

  processor::PurchaseIntent processor(&resource);

  const GURL url_1 = GURL("https://www.brave.com/test?foo=bar");
  processor.Process(url_1);

  AdvanceClockBy(base::Days(1));

  const GURL url_2 = GURL("https://www.basicattentiontoken.org/test?bar=foo");
  processor.Process(url_2);

  const PurchaseIntent model;

  // Act
  const SegmentList segments = model.GetSegments();

  // Assert
  EXPECT_TRUE(segments.empty());
}

TEST_F(BatAdsPurchaseIntentModelTest, DoNotGetSegmentsIfNeverProcessed) {
  // Arrange
  resource::PurchaseIntent resource;
  resource.Load();
  task_environment_.RunUntilIdle();

  const PurchaseIntent model;

  // Act
  const SegmentList segments = model.GetSegments();

  // Assert
  EXPECT_TRUE(segments.empty());
}

TEST_F(BatAdsPurchaseIntentModelTest,
       DoNotGetSegmentsIfNeverMatchedFunnelSites) {
  // Arrange
  resource::PurchaseIntent resource;
  resource.Load();
  task_environment_.RunUntilIdle();

  processor::PurchaseIntent processor(&resource);

  const GURL url = GURL("https://duckduckgo.com/?q=segment+keyword+1");
  processor.Process(url);

  const PurchaseIntent model;

  // Act
  const SegmentList segments = model.GetSegments();

  // Assert
  EXPECT_TRUE(segments.empty());
}

TEST_F(BatAdsPurchaseIntentModelTest, GetSegmentsForPreviouslyMatchedSite) {
  // Arrange
  resource::PurchaseIntent resource;
  resource.Load();
  task_environment_.RunUntilIdle();

  processor::PurchaseIntent processor(&resource);

  const GURL url_1 = GURL("https://www.brave.com/test?foo=bar");
  processor.Process(url_1);

  const GURL url_2 = GURL("https://www.basicattentiontoken.org/test?bar=foo");
  processor.Process(url_2);

  processor.Process(url_1);

  const PurchaseIntent model;

  // Act
  const SegmentList segments = model.GetSegments();

  // Assert
  const SegmentList expected_segments = {"segment 3", "segment 2"};
  EXPECT_EQ(expected_segments, segments);
}

TEST_F(BatAdsPurchaseIntentModelTest,
       GetSegmentsForPreviouslyMatchedSegmentKeywords) {
  // Arrange
  resource::PurchaseIntent resource;
  resource.Load();
  task_environment_.RunUntilIdle();

  processor::PurchaseIntent processor(&resource);

  const GURL url = GURL("https://duckduckgo.com/?q=segment+keyword+1&foo=bar");
  processor.Process(url);
  processor.Process(url);
  processor.Process(url);

  const PurchaseIntent model;

  // Act
  const SegmentList segments = model.GetSegments();

  // Assert
  const SegmentList expected_segments = {"segment 1"};
  EXPECT_EQ(expected_segments, segments);
}

TEST_F(BatAdsPurchaseIntentModelTest,
       GetSegmentsForPreviouslyMatchedFunnelKeywords) {
  // Arrange
  resource::PurchaseIntent resource;
  resource.Load();
  task_environment_.RunUntilIdle();

  processor::PurchaseIntent processor(&resource);

  const GURL url =
      GURL("https://duckduckgo.com/?q=segment+keyword+1+funnel+keyword+2");
  processor.Process(url);

  const PurchaseIntent model;

  // Act
  const SegmentList segments = model.GetSegments();

  // Assert
  const SegmentList expected_segments = {"segment 1"};
  EXPECT_EQ(expected_segments, segments);
}

}  // namespace brave_ads::targeting::model
