/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/css/local_font_face_source.h"

#include "brave/third_party/blink/renderer/core/farbling/brave_session_cache.h"

#define IsLocalFontAvailable IsLocalFontAvailable_ChromiumImpl

#include "src/third_party/blink/renderer/core/css/local_font_face_source.cc"

#undef IsLocalFontAvailable

namespace blink {

bool LocalFontFaceSource::IsLocalFontAvailable(
    const FontDescription& font_description) const {
  if (!brave::AllowFontFamily(font_selector_->GetExecutionContext(),
                              font_name_)) {
    return false;
  }

  return IsLocalFontAvailable_ChromiumImpl(font_description);
}

}  // namespace blink
