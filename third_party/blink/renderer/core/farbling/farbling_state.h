/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_FARBLING_FARBLING_STATE_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_FARBLING_FARBLING_STATE_H_

#include "base/token.h"
#include "brave/third_party/blink/renderer/brave_farbling_constants.h"
#include "third_party/blink/renderer/core/core_export.h"

namespace brave {

class CORE_EXPORT FarblingState {
 public:
  FarblingState();
  FarblingState(const FarblingState&);
  FarblingState& operator=(const FarblingState&);

  base::Token token() const { return token_; }
  BraveFarblingLevel base_level() const { return base_level_; }
  BraveFarblingLevel font_level() const { return font_level_; }
  bool is_reduce_language_enabled() const {
    return is_reduce_language_enabled_;
  }

  void set_token(base::Token token) { token_ = token; }
  void set_base_level(BraveFarblingLevel level) { base_level_ = level; }
  void set_font_level(BraveFarblingLevel level) { font_level_ = level; }
  void set_is_reduce_language_enabled(bool is_enabled) {
    is_reduce_language_enabled_ = is_enabled;
  }

 private:
  base::Token token_;
  BraveFarblingLevel base_level_ = BraveFarblingLevel::OFF;
  BraveFarblingLevel font_level_ = BraveFarblingLevel::OFF;
  bool is_reduce_language_enabled_ = false;
};

}  // namespace brave

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_FARBLING_FARBLING_STATE_H_
