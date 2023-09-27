/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/blink_converters.h"

#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/html/canvas/canvas_rendering_context.h"
#include "third_party/blink/renderer/core/html/canvas/text_metrics.h"

namespace blink {

template <>
String ToPageGraphBlinkArg(
    const bindings::NativeValueTraitsStringAdapter& adapter) {
  return String(adapter);
}

template <>
String ToPageGraphBlinkArg(const ScriptValue& script_value) {
  if (script_value.IsEmpty()) {
    return String();
  }
  if (script_value.IsNull()) {
    return "null";
  }
  if (script_value.IsUndefined()) {
    return "undefined";
  }
  if (script_value.IsFunction()) {
    return "function";
  }
  if (script_value.IsObject()) {
    return "object";
  }
  String result;
  if (!script_value.ToString(result)) {
    result = "ScriptValue::ToString failed";
  }
  return result;
}

String ToPageGraphBlinkArg(TextMetrics* result) {
  std::stringstream result_buffer;
  result_buffer
      << "width: " << result->width()
      << ", actualBoundingBoxLeft: " << result->actualBoundingBoxLeft()
      << ", actualBoundingBoxRight: " << result->actualBoundingBoxRight()
      << ", fontBoundingBoxAscent: " << result->fontBoundingBoxAscent()
      << ", fontBoundingBoxDescent: " << result->fontBoundingBoxDescent()
      << ", actualBoundingBoxAscent: " << result->actualBoundingBoxAscent()
      << ", actualBoundingBoxDescent: " << result->actualBoundingBoxDescent()
      << ", emHeightAscent: " << result->emHeightAscent()
      << ", emHeightDescent: " << result->emHeightDescent();

  result_buffer << ", hangingBaseline: " << result->hangingBaseline()
                << ", alphabeticBaseline: " << result->alphabeticBaseline()
                << ", ideographicBaseline: " << result->ideographicBaseline();

  return String(result_buffer.str());
}

String ToPageGraphBlinkArg(CanvasRenderingContext* context) {
  if (!context) {
    return "<null>";
  }

  switch (context->GetRenderingAPI()) {
    case CanvasRenderingContext::CanvasRenderingAPI::kUnknown:
      return "CanvasRenderingContext: unknown";
    case CanvasRenderingContext::CanvasRenderingAPI::k2D:
      return "CanvasRenderingContext: 2d";
    case CanvasRenderingContext::CanvasRenderingAPI::kWebgl:
      return "CanvasRenderingContext: webgl";
    case CanvasRenderingContext::CanvasRenderingAPI::kWebgl2:
      return "CanvasRenderingContext: webgl2";
    case CanvasRenderingContext::CanvasRenderingAPI::kBitmaprenderer:
      return "CanvasRenderingContext: bitmaprenderer";
    case CanvasRenderingContext::CanvasRenderingAPI::kWebgpu:
      return "CanvasRenderingContext: webgpu";
  }
}

PageGraphBlinkReceiverData ToPageGraphBlinkReceiverData(Document* document) {
  return {
      {"cookie_url", document->CookieURL().GetString()},
  };
}

}  // namespace blink
