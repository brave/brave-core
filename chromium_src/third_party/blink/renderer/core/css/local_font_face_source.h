/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_CSS_LOCAL_FONT_FACE_SOURCE_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_CSS_LOCAL_FONT_FACE_SOURCE_H_

#include "third_party/blink/renderer/core/css/css_font_face_source.h"

#define IsLocalFontAvailable                                       \
  IsLocalFontAvailable_ChromiumImpl(const FontDescription&) const; \
  bool IsLocalFontAvailable

#include "src/third_party/blink/renderer/core/css/local_font_face_source.h"  // IWYU pragma: export

#undef IsLocalFontAvailable

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_CSS_LOCAL_FONT_FACE_SOURCE_H_
