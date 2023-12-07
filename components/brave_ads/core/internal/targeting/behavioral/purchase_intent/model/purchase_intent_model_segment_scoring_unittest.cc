/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/model/purchase_intent_model_segment_scoring.h"

#include <vector>

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/client/client_state_manager.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/model/purchase_intent_model.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/model/purchase_intent_signal_info.h"
#include "brave/components/brave_ads/core/internal/targeting/behavioral/purchase_intent/purchase_intent_feature.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsPurchaseIntentModelSegmentScoringTest : public UnitTestBase {};

TEST_F(BraveAdsPurchaseIntentModelSegmentScoringTest,
       ComputePurchaseIntentSignalHistorySegmentScores) {
  // Arrange
  const base::Time decayed_at = Now() - kPurchaseIntentTimeWindow.Get();

  const base::Time on_cusp_at =
      Now() - (kPurchaseIntentTimeWindow.Get() - base::Milliseconds(1));

  const std::vector<PurchaseIntentSignalInfo> signals = {
      {decayed_at, {"segment 3"}, 1},
      {on_cusp_at, {"segment 4"}, 4},
      {Now() - base::Minutes(2), {"segment 1", "segment 2"}, 3},
      {Now() - base::Minutes(1), {"segment 1"}, 2},
      {Now(), {"segment 5"}, 5}};

  for (const auto& signal : signals) {
    BuyPurchaseIntentSignal(signal);
  }

  const PurchaseIntentSignalHistoryMap& signal_history =
      ClientStateManager::GetInstance().GetPurchaseIntentSignalHistory();

  // Act & Assert
  const std::multimap</*score*/ int, /*segment*/ std::string>
      expected_segment_scores = {{0, "segment 3"},
                                 {3, "segment 2"},
                                 {4, "segment 4"},
                                 {5, "segment 1"},
                                 {5, "segment 5"}};
  EXPECT_EQ(expected_segment_scores,
            ComputePurchaseIntentSignalHistorySegmentScores(signal_history));
}

TEST_F(BraveAdsPurchaseIntentModelSegmentScoringTest,
       ComputeEmptyPurchaseIntentSignalHistorySegmentScores) {
  // Act & Assert
  EXPECT_THAT(
      ComputePurchaseIntentSignalHistorySegmentScores(/*signal_history=*/{}),
      ::testing::IsEmpty());
}

}  // namespace brave_ads
