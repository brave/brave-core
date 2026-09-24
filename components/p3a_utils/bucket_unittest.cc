// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/p3a_utils/bucket.h"

#include "base/test/metrics/histogram_tester.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace p3a_utils {

TEST(P3AUtilsBucketTest, RecordPercentageHistogram) {
  constexpr char kHistogramName[] = "Brave.P3AUtils.Percentage";
  constexpr int kBuckets[] = {0, 1, 20, 80, 100};
  base::HistogramTester histogram_tester;

  RecordPercentageHistogram(kHistogramName, kBuckets, 1, 200);
  RecordPercentageHistogram(kHistogramName, kBuckets, 50, 100);
  RecordPercentageHistogram(kHistogramName, kBuckets, 1, 0);

  histogram_tester.ExpectBucketCount(kHistogramName, 1, 1);
  histogram_tester.ExpectBucketCount(kHistogramName, 3, 1);
  histogram_tester.ExpectTotalCount(kHistogramName, 2);
}

}  // namespace p3a_utils
