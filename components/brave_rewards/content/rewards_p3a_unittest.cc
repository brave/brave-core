/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/content/rewards_p3a.h"

#include "base/test/metrics/histogram_tester.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_rewards::p3a {

// Brave.Rewards.TipsSent.2 is recorded by both RecordTipsSent() and
// RecordNoWalletCreatedForAllMetrics(), and both must derive the same
// bucket count from kTipsSentBuckets. If they ever disagree,
// base::Histogram::Factory::Build() detects the mismatched construction
// arguments and calls DumpWithoutCrashing() (see base/metrics/histogram.cc),
// which is what the Android crash reports surfaced.
// See https://github.com/brave/brave-browser/issues/60255.
TEST(RewardsP3ATest, TipsSentHistogramParametersMatch) {
  base::HistogramTester histogram_tester;

  RecordTipsSent(1);
  RecordNoWalletCreatedForAllMetrics();

  histogram_tester.ExpectTotalCount("Histogram.MismatchedConstructionArguments",
                                    0);
  // If mismatched, the second call's sample is silently dropped by
  // DummyHistogram instead of being recorded under kTipsSentHistogramName.
  histogram_tester.ExpectTotalCount(kTipsSentHistogramName, 2);
}

}  // namespace brave_rewards::p3a
