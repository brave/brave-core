/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_DOM_DOM_HIGH_RES_TIME_STAMP_H_

#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_DOM_DOM_HIGH_RES_TIME_STAMP_H_

#define DOMHighResTimeStamp DOMHighResTimeStamp_ChromiumImpl

#define ConvertDOMHighResTimeStampToSeconds \
  ConvertDOMHighResTimeStampToSeconds_ChromiumImpl

#include "src/third_party/blink/renderer/core/dom/dom_high_res_time_stamp.h"

#undef DOMHighResTimeStamp
#undef ConvertDOMHighResTimeStampToSeconds

#include "brave/third_party/blink/renderer/core/dom/dom_high_res_time_stamp_feature.h"

namespace blink {

class DOMHighResTimeStamp {
 public:
  constexpr DOMHighResTimeStamp(int rhs)  // NOLINT(runtime/explicit)
      : value_(rhs) {}
  constexpr DOMHighResTimeStamp(double rhs)  // NOLINT(runtime/explicit)
      : value_(rhs) {}
  operator double() const { return GetValue(); }
  void operator=(double rhs) { value_ = rhs; }
  void operator=(int rhs) { value_ = rhs; }

  double GetValue() const {
    bool should_round = brave::IsTimeStampRoundingEnabled();
    return should_round ? round(value_) : value_;
  }

 private:
  double value_;
};

inline double ConvertDOMHighResTimeStampToSeconds(
    DOMHighResTimeStamp milliseconds) {
  return ConvertDOMHighResTimeStampToSeconds_ChromiumImpl(milliseconds);
}

}  // namespace blink

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_DOM_DOM_HIGH_RES_TIME_STAMP_H_
