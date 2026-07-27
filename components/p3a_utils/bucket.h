// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_P3A_UTILS_BUCKET_H_
#define BRAVE_COMPONENTS_P3A_UTILS_BUCKET_H_

#include <algorithm>

#include "base/check.h"
#include "base/check_op.h"
#include "base/metrics/histogram_functions.h"

namespace p3a_utils {

template <std::size_t N>
int BucketIndex(const int (&buckets)[N], int value) {
  DCHECK_GE(value, 0);
  return std::lower_bound(buckets, std::end(buckets), value) - buckets;
}

template <std::size_t N>
void RecordToHistogramBucket(const char* histogram_name,
                             const int (&buckets)[N],
                             int value) {
  DCHECK(histogram_name);
  base::UmaHistogramExactLinear(histogram_name, BucketIndex(buckets, value),
                                std::size(buckets) + 1);
}

template <std::size_t N>
void RecordPercentageHistogram(const char* histogram_name,
                               const int (&buckets)[N],
                               int numerator,
                               int denominator) {
  DCHECK(histogram_name);
  if (denominator <= 0) {
    return;
  }
  // Don't round down to 0% if the numerator is nonzero.
  int percentage =
      numerator > 0 ? std::max(1, (numerator * 100) / denominator) : 0;
  RecordToHistogramBucket(histogram_name, buckets, percentage);
}

}  // namespace p3a_utils

#endif  // BRAVE_COMPONENTS_P3A_UTILS_BUCKET_H_
