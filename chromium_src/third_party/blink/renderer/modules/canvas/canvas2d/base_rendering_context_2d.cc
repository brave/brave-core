/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/modules/canvas/canvas2d/base_rendering_context_2d.h"

#include "base/notreached.h"
#include "brave/third_party/blink/renderer/core/farbling/brave_session_cache.h"
#include "third_party/blink/renderer/platform/graphics/image_data_buffer.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"

namespace {

bool IsGoogleMaps(const blink::KURL& url) {
  const auto host = url.Host();
  if (!host.StartsWith("google.") && !host.Contains(".google.")) {
    return false;
  }
  const auto path = url.GetPath();
  return path == "/maps" || path.ToString().StartsWith("/maps/");
}

}  // namespace

#define BRAVE_GET_IMAGE_DATA                                              \
  if (ExecutionContext* context = ExecutionContext::From(script_state)) { \
    if (!IsGoogleMaps(context->Url())) {                                  \
      SkPixmap image_data_pixmap = image_data->GetSkPixmap();             \
      brave::BraveSessionCache::From(*context).PerturbPixels(             \
          static_cast<const unsigned char*>(                              \
              image_data_pixmap.writable_addr()),                         \
          image_data_pixmap.computeByteSize());                           \
    }                                                                     \
  }

#define BRAVE_BASE_RENDERING_CONTEXT_2D_MEASURE_TEXT      \
  if (!brave::AllowFingerprinting(                        \
          GetTopExecutionContext(),                       \
          ContentSettingsType::BRAVE_WEBCOMPAT_LANGUAGE)) \
    return MakeGarbageCollected<TextMetrics>();

#define BRAVE_GET_IMAGE_DATA_PARAMS ScriptState *script_state,
#define getImageData getImageData_Unused
#include "src/third_party/blink/renderer/modules/canvas/canvas2d/base_rendering_context_2d.cc"
#undef getImageData
#undef BRAVE_GET_IMAGE_DATA_PARAMS
#undef BRAVE_GET_IMAGE_DATA
#undef BRAVE_BASE_RENDERING_CONTEXT_2D_MEASURE_TEXT

namespace {

bool AllowFingerprintingFromScriptState(blink::ScriptState* script_state) {
  return brave::AllowFingerprinting(
      blink::ExecutionContext::From(script_state),
      ContentSettingsType::BRAVE_WEBCOMPAT_CANVAS);
}

}  // namespace

namespace blink {

ImageData* BaseRenderingContext2D::getImageData(
    int sx,
    int sy,
    int sw,
    int sh,
    ExceptionState& exception_state) {
  NOTREACHED_IN_MIGRATION();
  return nullptr;
}

ImageData* BaseRenderingContext2D::getImageData(
    int sx,
    int sy,
    int sw,
    int sh,
    ImageDataSettings* image_data_settings,
    ExceptionState& exception_state) {
  NOTREACHED_IN_MIGRATION();
  return nullptr;
}

ImageData* BaseRenderingContext2D::getImageDataInternal(
    int sx,
    int sy,
    int sw,
    int sh,
    ImageDataSettings* image_data_settings,
    ExceptionState& exception_state) {
  NOTREACHED_IN_MIGRATION();
  return nullptr;
}

ImageData* BaseRenderingContext2D::getImageDataInternal_Unused(
    int sx,
    int sy,
    int sw,
    int sh,
    ImageDataSettings* image_data_settings,
    ExceptionState& exception_state) {
  NOTREACHED_IN_MIGRATION();
  return nullptr;
}

ImageData* BaseRenderingContext2D::getImageData(
    ScriptState* script_state,
    int sx,
    int sy,
    int sw,
    int sh,
    ExceptionState& exception_state) {
  return getImageDataInternal(script_state, sx, sy, sw, sh,
                              /*image_data_settings=*/nullptr, exception_state);
}

ImageData* BaseRenderingContext2D::getImageData(
    ScriptState* script_state,
    int sx,
    int sy,
    int sw,
    int sh,
    ImageDataSettings* image_data_settings,
    ExceptionState& exception_state) {
  return getImageDataInternal(script_state, sx, sy, sw, sh, image_data_settings,
                              exception_state);
}

bool BaseRenderingContext2D::isPointInPath(ScriptState* script_state,
                                           const double x,
                                           const double y,
                                           const String& winding_rule_string) {
  if (!AllowFingerprintingFromScriptState(script_state))
    return false;
  return isPointInPath(x, y, winding_rule_string);
}

bool BaseRenderingContext2D::isPointInPath(ScriptState* script_state,
                                           Path2D* dom_path,
                                           const double x,
                                           const double y,
                                           const String& winding_rule_string) {
  if (!AllowFingerprintingFromScriptState(script_state))
    return false;
  return isPointInPath(dom_path, x, y, winding_rule_string);
}

bool BaseRenderingContext2D::isPointInStroke(ScriptState* script_state,
                                             const double x,
                                             const double y) {
  if (!AllowFingerprintingFromScriptState(script_state))
    return false;
  return isPointInStroke(x, y);
}

bool BaseRenderingContext2D::isPointInStroke(ScriptState* script_state,
                                             Path2D* dom_path,
                                             const double x,
                                             const double y) {
  if (!AllowFingerprintingFromScriptState(script_state))
    return false;
  return isPointInStroke(dom_path, x, y);
}

}  // namespace blink
