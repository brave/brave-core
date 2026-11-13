/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/modules/canvas/canvas2d/base_rendering_context_2d.h"

#include "brave/third_party/blink/renderer/core/farbling/brave_session_cache.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/html/canvas/image_data.h"
#include "third_party/blink/renderer/platform/bindings/script_state.h"
#include "ui/gfx/skia_span_util.h"

#include <third_party/blink/renderer/modules/canvas/canvas2d/base_rendering_context_2d.cc>

namespace blink {

namespace {

ImageData* FarbleImageData(ScriptState* script_state, ImageData* image_data) {
  if (!image_data) {
    return nullptr;
  }
  if (ExecutionContext* context = ExecutionContext::From(script_state)) {
    SkPixmap image_data_pixmap = image_data->GetSkPixmap();
    brave::BraveSessionCache::From(*context).PerturbPixels(
        gfx::SkPixmapToWritableSpan(image_data_pixmap));
  }
  return image_data;
}

}  // namespace

ImageData* BaseRenderingContext2D::getImageData(
    ScriptState* script_state,
    int sx,
    int sy,
    int sw,
    int sh,
    ExceptionState& exception_state) {
  return FarbleImageData(script_state,
                         getImageData(sx, sy, sw, sh, exception_state));
}

ImageData* BaseRenderingContext2D::getImageData(
    ScriptState* script_state,
    int sx,
    int sy,
    int sw,
    int sh,
    ImageDataSettings* image_data_settings,
    ExceptionState& exception_state) {
  return FarbleImageData(
      script_state,
      getImageData(sx, sy, sw, sh, image_data_settings, exception_state));
}

}  // namespace blink
