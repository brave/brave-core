/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <sstream>

#include "../gen/third_party/blink/renderer/bindings/modules/v8/v8_canvas_context_creation_attributes_module.cc"

namespace blink {

MODULES_EXPORT String
ToPageGraphBlinkArg(CanvasContextCreationAttributesModule* attributes) {
  std::stringstream buffer;
  buffer << "alpha: " << attributes->alpha()
         << ", antialias: " << attributes->antialias()
         << ", color_space: " << attributes->colorSpace().AsString()
         << ", depth: " << attributes->depth()
         << ", fail_if_major_performance_caveat: "
         << attributes->failIfMajorPerformanceCaveat()
         << ", desynchronized: " << attributes->desynchronized()
         << ", pixel_format: " << attributes->pixelFormat().AsString()
         << ", premultiplied_alpha: " << attributes->premultipliedAlpha()
         << ", preserve_drawing_buffer: " << attributes->preserveDrawingBuffer()
         << ", power_preference: " << attributes->powerPreference().AsString()
         << ", stencil: " << attributes->stencil()
         << ", xr_compatible: " << attributes->xrCompatible();
  return String(buffer.str());
}

}  // namespace blink
