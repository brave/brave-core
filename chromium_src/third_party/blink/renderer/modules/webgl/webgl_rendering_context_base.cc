/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/public/platform/web_content_settings_client.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/workers/worker_global_scope.h"

namespace {

using blink::DynamicTo;
using blink::ExecutionContext;
using blink::LocalDOMWindow;
using blink::ScriptState;
using blink::To;
using blink::WebContentSettingsClient;
using blink::WorkerGlobalScope;

bool AllowFingerprintingFromExecutionContext(ExecutionContext* context) {
  WebContentSettingsClient* settings = nullptr;
  if (auto* window = DynamicTo<LocalDOMWindow>(context))
    settings = window->GetFrame()->GetContentSettingsClient();
  else if (context->IsWorkerGlobalScope())
    settings = To<WorkerGlobalScope>(context)->ContentSettingsClient();
  return !settings || settings->AllowFingerprinting(true);
}

}  // namespace

#define BRAVE_WEBGL_RENDERING_CONTEXT_BASE_RETURN         \
  if (Host() && !AllowFingerprintingFromExecutionContext( \
                    Host()->GetTopExecutionContext()))    \
    return;

#define BRAVE_WEBGL_RENDERING_CONTEXT_BASE_NULLPTR        \
  if (Host() && !AllowFingerprintingFromExecutionContext( \
                    Host()->GetTopExecutionContext()))    \
    return nullptr;

#define BRAVE_WEBGL_RENDERING_CONTEXT_BASE_NULLOPT        \
  if (Host() && !AllowFingerprintingFromExecutionContext( \
                    Host()->GetTopExecutionContext()))    \
    return base::nullopt;

#define BRAVE_WEBGL_RENDERING_CONTEXT_BASE_ZERO           \
  if (Host() && !AllowFingerprintingFromExecutionContext( \
                    Host()->GetTopExecutionContext()))    \
    return 0;

#define BRAVE_WEBGL_RENDERING_CONTEXT_BASE_MINUS_ONE      \
  if (Host() && !AllowFingerprintingFromExecutionContext( \
                    Host()->GetTopExecutionContext()))    \
    return -1;

#define BRAVE_WEBGL_RENDERING_CONTEXT_BASE_SCRIPT_VALUE   \
  if (Host() && !AllowFingerprintingFromExecutionContext( \
                    Host()->GetTopExecutionContext()))    \
    return ScriptValue::CreateNull(script_state->GetIsolate());

#define BRAVE_WEBGL_RENDERING_CONTEXT_BASE_STRING         \
  if (Host() && !AllowFingerprintingFromExecutionContext( \
                    Host()->GetTopExecutionContext()))    \
    return String();

#define BRAVE_WEBGL_RENDERING_CONTEXT_BASE_GETSHADERINFOLOG \
  range[0] = 0;                                             \
  range[1] = 0;                                             \
  precision = 0;

#define BRAVE_WEBGL_GET_PARAMETER_UNMASKED_RENDERER              \
  if (ExtensionEnabled(kWebGLDebugRendererInfoName) && Host() && \
      !AllowFingerprintingFromExecutionContext(                  \
          Host()->GetTopExecutionContext()))                     \
    return WebGLAny(                                             \
        script_state,                                            \
        String(brave::BraveSessionCache::From(                   \
                   *(Host()->GetTopExecutionContext()))          \
                   .GenerateRandomString("UNMASKED_RENDERER_WEBGL", 8)));

#define BRAVE_WEBGL_GET_PARAMETER_UNMASKED_VENDOR                \
  if (ExtensionEnabled(kWebGLDebugRendererInfoName) && Host() && \
      !AllowFingerprintingFromExecutionContext(                  \
          Host()->GetTopExecutionContext()))                     \
    return WebGLAny(                                             \
        script_state,                                            \
        String(brave::BraveSessionCache::From(                   \
                   *(Host()->GetTopExecutionContext()))          \
                   .GenerateRandomString("UNMASKED_VENDOR_WEBGL", 8)));

#include "../../../../../../../third_party/blink/renderer/modules/webgl/webgl_rendering_context_base.cc"
#undef BRAVE_WEBGL_GET_PARAMETER_UNMASKED_RENDERER
#undef BRAVE_WEBGL_GET_PARAMETER_UNMASKED_VENDOR
#undef BRAVE_WEBGL_RENDERING_CONTEXT_BASE_STRING
#undef BRAVE_WEBGL_RENDERING_CONTEXT_BASE_SCRIPT_VALUE
#undef BRAVE_WEBGL_RENDERING_CONTEXT_BASE_MINUS_ONE
#undef BRAVE_WEBGL_RENDERING_CONTEXT_BASE_ZERO
#undef BRAVE_WEBGL_RENDERING_CONTEXT_BASE_NULLOPT
#undef BRAVE_WEBGL_RENDERING_CONTEXT_BASE_NULLPTR
#undef BRAVE_WEBGL_RENDERING_CONTEXT_BASE_RETURN
