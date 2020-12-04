/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/wip/ad_targeting/resources/purchase_intent/purchase_intent_resource.h"

#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace ad_targeting {
namespace resource {

class BatAdsTextClassificationTest : public UnitTestBase {
 protected:
  BatAdsTextClassificationTest() = default;

  ~BatAdsTextClassificationTest() override = default;
};

TEST_F(BatAdsTextClassificationTest,
    LoadForId) {
  // Arrange

  // Act
  TextClassification text_classification;
  text_classification.LoadForId("...");

  // Assert
  FAIL();
}

}  // namespace resource
}  // namespace ad_targeting
}  // namespace ads
