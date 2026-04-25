/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/model/purchase_intent_model_segment_predictor.h"

#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/model/purchase_intent_model.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/model/purchase_intent_model_segment_scoring.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/model/purchase_intent_signal_info.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/purchase_intent_feature.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/resource/purchase_intent_signal_history_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsPurchaseIntentModelSegmentPredictorTest : public test::TestBase {
};

TEST_F(BraveAdsPurchaseIntentModelSegmentPredictorTest,
       PredictPurchaseIntentSegments) {
  // Arrange
  const base::TimeDelta purchase_intent_time_window =
      kPurchaseIntentTimeWindow.Get();

  const PurchaseIntentSignalList purchase_intent_signals = {
      {/*decayed_at*/ test::Now() - purchase_intent_time_window,
       {"segment 3"},
       /*weight*/ 1},
      {/*on_cusp_at*/ test::Now() -
           (purchase_intent_time_window - base::Milliseconds(1)),
       {"segment 4"},
       /*weight*/ 4},
      {/*at*/ test::Now() - base::Minutes(2),
       {"segment 1", "segment 2"},
       /*weight*/ 3},
      {/*at*/ test::Now() - base::Minutes(1), {"segment 1"}, /*weight*/ 2},
      {/*at*/ test::Now(), {"segment 5"}, /*weight*/ 5}};

  for (const auto& purchase_intent_signal : purchase_intent_signals) {
    BuyPurchaseIntentSignal(purchase_intent_signal);
  }

  const PurchaseIntentSignalHistoryMap& purchase_intent_signal_history =
      ClientStateManager::GetInstance().GetPurchaseIntentSignalHistory();

  const std::multimap</*score*/ int, /*segment*/ std::string>
      purchase_intent_signal_history_segment_scores =
          ComputePurchaseIntentSignalHistorySegmentScores(
              purchase_intent_signal_history);

  // Act
  const SegmentList purchase_intent_segments = PredictPurchaseIntentSegments(
      purchase_intent_signal_history_segment_scores);

  // Assert
  const SegmentList expected_purchase_intent_segments = {
      "segment 5", "segment 1", "segment 4"};
  EXPECT_EQ(expected_purchase_intent_segments, purchase_intent_segments);
}

TEST_F(BraveAdsPurchaseIntentModelSegmentPredictorTest,
       DoNotPredictPurchaseIntentSegmentsWhenNoScores) {
  // Act
  const SegmentList purchase_intent_segments =
      PredictPurchaseIntentSegments(/*segment_scores=*/{});

  // Assert
  EXPECT_THAT(purchase_intent_segments, ::testing::IsEmpty());
}

}  // namespace brave_ads
