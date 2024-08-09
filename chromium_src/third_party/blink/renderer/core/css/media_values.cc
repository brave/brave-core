/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/css/media_values.h"

#include "brave/third_party/blink/renderer/core/farbling/brave_session_cache.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/screen.h"

#define CalculateDeviceWidth(...)                                              \
  CalculateDeviceWidth(__VA_ARGS__, bool early) {                              \
    ExecutionContext* context = frame->DomWindow()->GetExecutionContext();     \
    auto* top_frame = DynamicTo<LocalFrame>(frame->Top());                     \
    return top_frame && brave::BlockScreenFingerprinting(context, early)       \
               ? brave::FarbleInteger(context,                                 \
                                      brave::FarbleKey::kWindowInnerWidth,     \
                                      CalculateViewportWidth(top_frame), 0, 8) \
               : CalculateDeviceWidth_ChromiumImpl(frame);                     \
  }                                                                            \
  int MediaValues::CalculateDeviceWidth_ChromiumImpl(__VA_ARGS__)

#define CalculateDeviceHeight(...)                                         \
  CalculateDeviceHeight(__VA_ARGS__, bool early) {                         \
    ExecutionContext* context = frame->DomWindow()->GetExecutionContext(); \
    auto* top_frame = DynamicTo<LocalFrame>(frame->Top());                 \
    return top_frame && brave::BlockScreenFingerprinting(context, early)   \
               ? brave::FarbleInteger(                                     \
                     context, brave::FarbleKey::kWindowInnerHeight,        \
                     CalculateViewportHeight(top_frame), 0, 8)             \
               : CalculateDeviceHeight_ChromiumImpl(frame);                \
  }                                                                        \
  int MediaValues::CalculateDeviceHeight_ChromiumImpl(__VA_ARGS__)

#include "src/third_party/blink/renderer/core/css/media_values.cc"

#undef CalculateDeviceWidth
#undef CalculateDeviceHeight
