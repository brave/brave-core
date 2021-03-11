/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_CSS_OFFSCREEN_FONT_SELECTOR_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_CSS_OFFSCREEN_FONT_SELECTOR_H_

#define BRAVE_OFFSCREEN_FONT_SELECTOR_H \
  bool AllowFontFamily(const AtomicString& family) override;

#include "src/third_party/blink/renderer/core/css/offscreen_font_selector.h"

#undef BRAVE_OFFSCREEN_FONT_SELECTOR_H

#endif
