/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/modules/canvas/canvas2d/canvas_rendering_context_2d.h"

#include "brave/components/content_settings/renderer/brave_content_settings_agent_impl_helper.h"

#define BRAVE_CANVAS_RENDERING_CONTEXT_2D_MEASURE_TEXT          \
  if (!AllowFingerprinting(canvas()->GetDocument().GetFrame())) \
    return MakeGarbageCollected<TextMetrics>();

#define getImageData getImageDataUnused
#include "../../../../../../../../third_party/blink/renderer/modules/canvas/canvas2d/canvas_rendering_context_2d.cc"
#undef getImageData
#undef BRAVE_CANVAS_RENDERING_CONTEXT_2D_MEASURE_TEXT

namespace blink {

ImageData* CanvasRenderingContext2D::getImageData(
    ScriptState* script_state,
    int sx,
    int sy,
    int sw,
    int sh,
    ExceptionState& exception_state) {
  blink::IdentifiabilityMetricBuilder(ukm_source_id_)
      .Set(blink::IdentifiableSurface::FromTypeAndInput(
               blink::IdentifiableSurface::Type::kCanvasReadback,
               GetContextType()),
           0)
      .Record(ukm_recorder_);
  return BaseRenderingContext2D::getImageData(script_state, sx, sy, sw, sh,
                                              exception_state);
}

}  // namespace blink
