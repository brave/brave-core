/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_BASE_METRICS_HISTOGRAM_FUNCTIONS_H_
#define BRAVE_CHROMIUM_SRC_BASE_METRICS_HISTOGRAM_FUNCTIONS_H_

// We use negative integers when defining Brave-specific PageActionIconType
// values to avoid conflicting with upstream values. However,
// UmaHistogramEnumeration performs a DCHECK_LE to confirm that all
// PageActionIconType values are less than T::kMaxValue and it casts the sample
// to an unsigned int, so we fail the DCHECK. This override only performs the
// DCHECK if the sample is non-negative.
#define BRAVE_HISTOGRAM_FUNCTIONS_UMA_HISTOGRAM_ENUMERATION \
  if (static_cast<intmax_t>(sample) >= 0)

#include "src/base/metrics/histogram_functions.h"  // IWYU pragma: export

#undef BRAVE_HISTOGRAM_FUNCTIONS_UMA_HISTOGRAM_ENUMERATION

#endif  // BRAVE_CHROMIUM_SRC_BASE_METRICS_HISTOGRAM_FUNCTIONS_H_
