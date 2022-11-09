/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/loader/document_load_timing.h"

#define MonotonicTimeToZeroBasedDocumentTime \
  MonotonicTimeToZeroBasedDocumentTime_ChromiumImpl

#include "src/third_party/blink/renderer/core/loader/document_load_timing.cc"

#undef MonotonicTimeToZeroBasedDocumentTime

#include "third_party/blink/renderer/core/timing/time_clamper.h"

namespace blink {

base::TimeDelta DocumentLoadTiming::MonotonicTimeToZeroBasedDocumentTime(
    base::TimeTicks monotonic_time) const {
  return TimeClamper::MaybeRoundTimeDelta(
      MonotonicTimeToZeroBasedDocumentTime_ChromiumImpl(monotonic_time));
}

}  // namespace blink
