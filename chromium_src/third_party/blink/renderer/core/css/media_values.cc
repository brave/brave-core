/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_CSS_MEDIA_VALUES_CC_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_CSS_MEDIA_VALUES_CC_

#include "third_party/blink/renderer/core/css/media_values.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/screen.h"

#include "brave/third_party/blink/renderer/core/brave_session_cache.h"

#define CalculateDeviceWidth                                               \
  CalculateDeviceWidth(LocalFrame* frame) {                                \
    ExecutionContext* context = frame->DomWindow()->GetExecutionContext(); \
    return brave::AllowScreenFingerprinting(context)                       \
               ? CalculateDeviceWidth_ChromiumImpl(frame)                  \
               : frame->DomWindow()->screen()->width();                    \
  }                                                                        \
  int MediaValues::CalculateDeviceWidth_ChromiumImpl

#define CalculateDeviceHeight                                              \
  CalculateDeviceHeight(LocalFrame* frame) {                               \
    ExecutionContext* context = frame->DomWindow()->GetExecutionContext(); \
    return brave::AllowScreenFingerprinting(context)                       \
               ? CalculateDeviceHeight_ChromiumImpl(frame)                 \
               : frame->DomWindow()->screen()->height();                   \
  }                                                                        \
  int MediaValues::CalculateDeviceHeight_ChromiumImpl

#include "src/third_party/blink/renderer/core/css/media_values.cc"

#undef CalculateDeviceWidth
#undef CalculateDeviceHeight

// BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_CSS_MEDIA_VALUES_CC_
#endif
