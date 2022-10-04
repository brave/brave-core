/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_VIDEO_RVFC_VIDEO_FRAME_CALLBACK_REQUESTER_IMPL_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_VIDEO_RVFC_VIDEO_FRAME_CALLBACK_REQUESTER_IMPL_H_

#include "third_party/blink/renderer/bindings/core/v8/script_promise_resolver.h"
#include "third_party/blink/renderer/modules/xr/xr_frame_provider.h"

#define OnImmersiveFrame                                                   \
  dummy();                                                                 \
  static double Test_GetCoarseClampedTimeInSeconds(base::TimeDelta time) { \
    return GetCoarseClampedTimeInSeconds(time);                            \
  }                                                                        \
  void OnImmersiveFrame

#include "src/third_party/blink/renderer/modules/video_rvfc/video_frame_callback_requester_impl.h"

#undef OnImmersiveFrame

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_VIDEO_RVFC_VIDEO_FRAME_CALLBACK_REQUESTER_IMPL_H_
