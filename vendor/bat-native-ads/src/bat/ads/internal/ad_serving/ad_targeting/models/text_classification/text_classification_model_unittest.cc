/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/wip/ad_serving/ad_targeting/models/text_classification/text_classification_model.h"

#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace ad_targeting {
namespace model {

class BatAdsTextClassificationModelTest : public UnitTestBase {
 protected:
  BatAdsTextClassificationModelTest() = default;

  ~BatAdsTextClassificationModelTest() override = default;
};

TEST_F(BatAdsTextClassificationModelTest,
    GetSegments) {
  // Arrange

  // Act
  TextClassificationModel model;
  const SegmentList segments = model.GetSegments();

  // Assert
  FAIL();
}

}  // namespace model
}  // namespace ad_targeting
}  // namespace ads
