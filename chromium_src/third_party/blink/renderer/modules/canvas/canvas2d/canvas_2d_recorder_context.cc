/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/modules/canvas/canvas2d/canvas_2d_recorder_context.h"

#include "brave/third_party/blink/renderer/core/farbling/brave_session_cache.h"

#include "src/third_party/blink/renderer/modules/canvas/canvas2d/canvas_2d_recorder_context.cc"

namespace {

bool AllowFingerprintingFromScriptState(blink::ScriptState* script_state) {
  return brave::AllowFingerprinting(
      blink::ExecutionContext::From(script_state),
      ContentSettingsType::BRAVE_WEBCOMPAT_CANVAS);
}

}  // namespace

namespace blink {

bool Canvas2DRecorderContext::isPointInPath(ScriptState* script_state,
                                            const double x,
                                            const double y,
                                            const V8CanvasFillRule& winding) {
  if (!AllowFingerprintingFromScriptState(script_state)) {
    return false;
  }
  return isPointInPath(x, y, winding);
}

bool Canvas2DRecorderContext::isPointInPath(ScriptState* script_state,
                                            Path2D* dom_path,
                                            const double x,
                                            const double y,
                                            const V8CanvasFillRule& winding) {
  if (!AllowFingerprintingFromScriptState(script_state)) {
    return false;
  }
  return isPointInPath(dom_path, x, y, winding);
}

bool Canvas2DRecorderContext::isPointInStroke(ScriptState* script_state,
                                              const double x,
                                              const double y) {
  if (!AllowFingerprintingFromScriptState(script_state)) {
    return false;
  }
  return isPointInStroke(x, y);
}

bool Canvas2DRecorderContext::isPointInStroke(ScriptState* script_state,
                                              Path2D* dom_path,
                                              const double x,
                                              const double y) {
  if (!AllowFingerprintingFromScriptState(script_state)) {
    return false;
  }
  return isPointInStroke(dom_path, x, y);
}

}  // namespace blink
