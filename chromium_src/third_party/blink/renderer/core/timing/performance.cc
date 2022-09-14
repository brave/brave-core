/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/timing/performance.h"

#define Milliseconds(A) Milliseconds(static_cast<double>(A))

#include "src/third_party/blink/renderer/core/timing/performance.cc"

#undef Milliseconds

namespace blink {

double Performance::nowAsDouble() const {
  return now();
}

double Performance::MonotonicTimeToDOMHighResTimeStampAsDouble(
    base::TimeTicks monotonic_time) const {
  return MonotonicTimeToDOMHighResTimeStamp(monotonic_time);
}

}  // namespace blink
