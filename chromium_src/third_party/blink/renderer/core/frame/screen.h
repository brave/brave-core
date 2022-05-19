/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_SCREEN_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_SCREEN_H_

#include "ui/display/screen_info.h"

#define GetScreenInfo                             \
  GetScreenInfo() const;                          \
  mutable display::ScreenInfo brave_screen_info_; \
  void dummy

#include "src/third_party/blink/renderer/core/frame/screen.h"

#undef GetScreenInfo

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_SCREEN_H_
