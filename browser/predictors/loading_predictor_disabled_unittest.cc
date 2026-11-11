/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Covers the effect of the plaster for
// chrome/browser/predictors/loading_predictor_config.cc, and replaces the
// upstream LoadingPredictorConfigTest cases it inverts.

#include "chrome/browser/predictors/loading_predictor_config.h"
#include "chrome/browser/predictors/loading_predictor_factory.h"
#include "chrome/browser/preloading/preloading_prefs.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace predictors {

class LoadingPredictorDisabledTest : public testing::Test {
 protected:
  Profile* profile() { return &profile_; }

  void SetPreloadPagesState(prefetch::PreloadPagesState state) {
    prefetch::SetPreloadPagesState(profile_.GetPrefs(), state);
  }

 private:
  content::BrowserTaskEnvironment task_environment_;
  TestingProfile profile_;
};

TEST_F(LoadingPredictorDisabledTest, NeverEnabled) {
  SetPreloadPagesState(prefetch::PreloadPagesState::kStandardPreloading);
  EXPECT_FALSE(IsLoadingPredictorEnabled(profile()));

  SetPreloadPagesState(prefetch::PreloadPagesState::kNoPreloading);
  EXPECT_FALSE(IsLoadingPredictorEnabled(profile()));
}

// Without a service there is no LoadingPredictorTabHelper, which is what keeps
// the predictor from learning and persisting network predictions.
TEST_F(LoadingPredictorDisabledTest, NoServiceEvenWhenPreloadingIsOptedInto) {
  SetPreloadPagesState(prefetch::PreloadPagesState::kStandardPreloading);

  EXPECT_FALSE(LoadingPredictorFactory::GetForProfile(profile()));
}

// The preload pref keeps its upstream meaning: it still gates preconnecting,
// which reaches the network stack without going through the predictor.
TEST_F(LoadingPredictorDisabledTest, PreconnectStillFollowsPref) {
  SetPreloadPagesState(prefetch::PreloadPagesState::kStandardPreloading);
  EXPECT_TRUE(IsPreconnectAllowed(profile()));

  SetPreloadPagesState(prefetch::PreloadPagesState::kNoPreloading);
  EXPECT_FALSE(IsPreconnectAllowed(profile()));
}

}  // namespace predictors
