/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/modules/canvas/canvas2d/base_rendering_context_2d.h"

#include "third_party/blink/public/platform/web_content_settings_client.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/workers/worker_global_scope.h"

#define BRAVE_GET_IMAGE_DATA                                              \
  if (ExecutionContext* context = ExecutionContext::From(script_state)) { \
    WebContentSettingsClient* settings = nullptr;                         \
    if (auto* window = DynamicTo<LocalDOMWindow>(context))                \
      settings = window->GetFrame()->GetContentSettingsClient();          \
    else if (context->IsWorkerGlobalScope())                              \
      settings = To<WorkerGlobalScope>(context)->ContentSettingsClient(); \
    if (settings) {                                                       \
      snapshot = brave::BraveSessionCache::From(*context).PerturbPixels(  \
          settings, snapshot);                                            \
    }                                                                     \
  }

#define BRAVE_GET_IMAGE_DATA_PARAMS ScriptState *script_state,
#include "../../../../../../../../third_party/blink/renderer/modules/canvas/canvas2d/base_rendering_context_2d.cc"
#undef BRAVE_GET_IMAGE_DATA_PARAMS
#undef BRAVE_GET_IMAGE_DATA

namespace {

using blink::DynamicTo;
using blink::ExecutionContext;
using blink::LocalDOMWindow;
using blink::ScriptState;
using blink::To;
using blink::WebContentSettingsClient;
using blink::WorkerGlobalScope;

bool AllowFingerprintingFromScriptState(ScriptState* script_state) {
  ExecutionContext* context = ExecutionContext::From(script_state);
  WebContentSettingsClient* settings = nullptr;
  if (auto* window = DynamicTo<LocalDOMWindow>(context))
    settings = window->GetFrame()->GetContentSettingsClient();
  else if (context->IsWorkerGlobalScope())
    settings = To<WorkerGlobalScope>(context)->ContentSettingsClient();
  return !settings || settings->AllowFingerprinting(true);
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
