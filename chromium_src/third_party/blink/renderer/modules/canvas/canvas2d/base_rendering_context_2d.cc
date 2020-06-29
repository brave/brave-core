/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/modules/canvas/canvas2d/base_rendering_context_2d.h"

#include "brave/components/content_settings/renderer/brave_content_settings_agent_impl_helper.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"

#define BRAVE_GET_IMAGE_DATA                                                 \
  LocalDOMWindow* window = LocalDOMWindow::From(script_state);               \
  if (window) {                                                              \
    snapshot = brave::BraveSessionCache::From(*(window->document()))         \
                   .PerturbPixels(window->document()->GetFrame(), snapshot); \
  }

#define BRAVE_GET_IMAGE_DATA_PARAMS ScriptState *script_state,
#include "../../../../../../../../third_party/blink/renderer/modules/canvas/canvas2d/base_rendering_context_2d.cc"
#undef BRAVE_GET_IMAGE_DATA_PARAMS
#undef BRAVE_GET_IMAGE_DATA

namespace {

bool AllowFingerprintingFromScriptState(blink::ScriptState* script_state) {
  blink::LocalDOMWindow* window = blink::LocalDOMWindow::From(script_state);
  return !window || AllowFingerprinting(window->GetFrame());
}

}  // namespace

namespace blink {

ImageData* BaseRenderingContext2D::getImageDataUnused(
    int sx,
    int sy,
    int sw,
    int sh,
    ExceptionState& exception_state) {
  return nullptr;
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
