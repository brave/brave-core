// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_P3A_UTILS_BUCKET_H_
#define BRAVE_COMPONENTS_P3A_UTILS_BUCKET_H_

#include <algorithm>

#include "base/metrics/histogram_functions.h"

namespace p3a_utils {

template <std::size_t N>
void RecordToHistogramBucket(const char* histogram_name,
                             const int (&buckets)[N],
                             int value) {
  DCHECK(histogram_name);
  DCHECK_GE(value, 0);
  const int* it_count = std::lower_bound(buckets, std::end(buckets), value);
  int answer = it_count - buckets;
  base::UmaHistogramExactLinear(histogram_name, answer, std::size(buckets) + 1);
}

}  // namespace p3a_utils

#endif  // BRAVE_COMPONENTS_P3A_UTILS_BUCKET_H_
