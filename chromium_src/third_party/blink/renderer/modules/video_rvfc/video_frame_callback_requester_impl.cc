/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/modules/video_rvfc/video_frame_callback_requester_impl.h"

#include "third_party/abseil-cpp/absl/container/internal/layout.h"
#include "third_party/blink/renderer/core/timing/time_clamper.h"

#define FloorToMultiple(X) \
  FloorToMultiple(         \
      base::Microseconds(TimeClamper::CoarseResolutionMicroseconds()))

#define kCoarseResolutionMicroseconds kCoarseResolutionMicroseconds_ChromiumImpl

#include "src/third_party/blink/renderer/modules/video_rvfc/video_frame_callback_requester_impl.cc"

#undef FloorToMultiple
#undef kCoarseResolutionMicroseconds
