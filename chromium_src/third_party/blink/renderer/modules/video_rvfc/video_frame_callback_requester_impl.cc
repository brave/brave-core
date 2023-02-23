/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/modules/video_rvfc/video_frame_callback_requester_impl.h"

#include "third_party/abseil-cpp/absl/container/internal/layout.h"

#define kCoarseResolution kCoarseResolution [[maybe_unused]]

#define FloorToMultiple(X) \
  FloorToMultiple(         \
      base::Microseconds(TimeClamper::CoarseResolutionMicroseconds()))

#define static_assert(...) static_assert(true)

#include "src/third_party/blink/renderer/modules/video_rvfc/video_frame_callback_requester_impl.cc"

#undef kCoarseResolution
#undef FloorToMultiple
#undef static_assert
