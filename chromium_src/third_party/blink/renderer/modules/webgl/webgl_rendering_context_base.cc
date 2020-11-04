/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/public/platform/web_content_settings_client.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/html/canvas/canvas_rendering_context_host.h"
#include "third_party/blink/renderer/core/workers/worker_global_scope.h"

namespace {

bool AllowFingerprintingForHost(blink::CanvasRenderingContextHost* host) {
  if (!host)
    return true;
  blink::ExecutionContext* context = host->GetTopExecutionContext();
  blink::WebContentSettingsClient* settings =
      brave::GetContentSettingsClientFor(context);
  return !settings || settings->AllowFingerprinting(true);
}

}  // namespace

#define BRAVE_WEBGL_RENDERING_CONTEXT_BASE_RETURN \
  if (!AllowFingerprintingForHost(Host()))        \
    return;

#define BRAVE_WEBGL_RENDERING_CONTEXT_BASE_NULLPTR \
  if (!AllowFingerprintingForHost(Host()))         \
    return nullptr;

#define BRAVE_WEBGL_RENDERING_CONTEXT_BASE_NULLOPT \
  if (!AllowFingerprintingForHost(Host()))         \
    return base::nullopt;

#define BRAVE_WEBGL_RENDERING_CONTEXT_BASE_ZERO \
  if (!AllowFingerprintingForHost(Host()))      \
    return 0;

#define BRAVE_WEBGL_RENDERING_CONTEXT_BASE_MINUS_ONE \
  if (!AllowFingerprintingForHost(Host()))           \
    return -1;

#define BRAVE_WEBGL_RENDERING_CONTEXT_BASE_SCRIPT_VALUE \
  if (!AllowFingerprintingForHost(Host()))              \
    return ScriptValue::CreateNull(script_state->GetIsolate());

#define BRAVE_WEBGL_RENDERING_CONTEXT_BASE_STRING \
  if (!AllowFingerprintingForHost(Host()))        \
    return String();

#define BRAVE_WEBGL_RENDERING_CONTEXT_BASE_GETSHADERINFOLOG \
  if (!AllowFingerprintingForHost(Host())) {                \
    range[0] = 0;                                           \
    range[1] = 0;                                           \
    precision = 0;                                          \
  }

#define BRAVE_WEBGL_GET_PARAMETER_UNMASKED_RENDERER     \
  if (ExtensionEnabled(kWebGLDebugRendererInfoName) &&  \
      !AllowFingerprintingForHost(Host()))              \
    return WebGLAny(                                    \
        script_state,                                   \
        String(brave::BraveSessionCache::From(          \
                   *(Host()->GetTopExecutionContext())) \
                   .GenerateRandomString("UNMASKED_RENDERER_WEBGL", 8)));

#define BRAVE_WEBGL_GET_PARAMETER_UNMASKED_VENDOR       \
  if (ExtensionEnabled(kWebGLDebugRendererInfoName) &&  \
      !AllowFingerprintingForHost(Host()))              \
    return WebGLAny(                                    \
        script_state,                                   \
        String(brave::BraveSessionCache::From(          \
                   *(Host()->GetTopExecutionContext())) \
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
