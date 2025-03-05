/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_CANVAS_CANVAS2D_CANVAS_2D_RECORDER_CONTEXT_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_CANVAS_CANVAS2D_CANVAS_2D_RECORDER_CONTEXT_H_

#define setGlobalCompositeOperation                                            \
  setGlobalCompositeOperation_unused();                                        \
  bool isPointInPath(ScriptState*, const double x, const double y,             \
                     const V8CanvasFillRule& winding =                         \
                         V8CanvasFillRule(V8CanvasFillRule::Enum::kNonzero));  \
  bool isPointInPath(ScriptState*, Path2D*, const double x, const double y,    \
                     const V8CanvasFillRule& winding =                         \
                         V8CanvasFillRule(V8CanvasFillRule::Enum::kNonzero));  \
  bool isPointInStroke(ScriptState*, const double x, const double y);          \
  bool isPointInStroke(ScriptState*, Path2D*, const double x, const double y); \
  void setGlobalCompositeOperation

#include "src/third_party/blink/renderer/modules/canvas/canvas2d/canvas_2d_recorder_context.h"  // IWYU pragma: export
#undef setGlobalCompositeOperation

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_CANVAS_CANVAS2D_CANVAS_2D_RECORDER_CONTEXT_H_
