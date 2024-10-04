/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_CANVAS_CANVAS2D_BASE_RENDERING_CONTEXT_2D_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_CANVAS_CANVAS2D_BASE_RENDERING_CONTEXT_2D_H_

// geImageDataInternal without ScriptState param is non-virtual and is used for
// calls from original getImage methods that will become getImage_Unused in the
// .cc file.
// getImageDataInternal_Unused is virtual for unused override in
// CanvasRenderingContext2D because we also replace that one with our own.
#define getImageDataInternal                                                   \
  getImageDataInternal(ScriptState*, int sx, int sy, int sw, int sh,           \
                       ImageDataSettings*, ExceptionState&);                   \
  ImageData* getImageDataInternal(int sx, int sy, int sw, int sh,              \
                                  ImageDataSettings*, ExceptionState&);        \
  ImageData* getImageData(ScriptState*, int sx, int sy, int sw, int sh,        \
                          ExceptionState&);                                    \
  ImageData* getImageData(ScriptState*, int sx, int sy, int sw, int sh,        \
                          ImageDataSettings*, ExceptionState&);                \
  ImageData* getImageData_Unused(int sx, int sy, int sw, int sh,               \
                                 ExceptionState&);                             \
  ImageData* getImageData_Unused(int sx, int sy, int sw, int sh,               \
                                 ImageDataSettings*, ExceptionState&);         \
                                                                               \
  bool isPointInPath(ScriptState*, const double x, const double y,             \
                     const V8CanvasFillRule& winding =                         \
                         V8CanvasFillRule(V8CanvasFillRule::Enum::kNonzero));  \
  bool isPointInPath(ScriptState*, Path2D*, const double x, const double y,    \
                     const V8CanvasFillRule& winding =                         \
                         V8CanvasFillRule(V8CanvasFillRule::Enum::kNonzero));  \
  bool isPointInStroke(ScriptState*, const double x, const double y);          \
  bool isPointInStroke(ScriptState*, Path2D*, const double x, const double y); \
                                                                               \
  virtual ImageData* getImageDataInternal_Unused

#include "src/third_party/blink/renderer/modules/canvas/canvas2d/base_rendering_context_2d.h"  // IWYU pragma: export
#undef getImageDataInternal

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_CANVAS_CANVAS2D_BASE_RENDERING_CONTEXT_2D_H_
