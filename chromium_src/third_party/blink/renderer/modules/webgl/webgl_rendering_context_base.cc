/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/content_settings/renderer/brave_content_settings_agent_impl_helper.h"
#include "third_party/blink/renderer/core/dom/document.h"

#define BRAVE_WEBGL_RENDERING_CONTEXT_BASE_RETURN                           \
  if (canvas() && !AllowFingerprinting(canvas()->GetDocument().GetFrame())) \
    return;

#define BRAVE_WEBGL_RENDERING_CONTEXT_BASE_NULLPTR                          \
  if (canvas() && !AllowFingerprinting(canvas()->GetDocument().GetFrame())) \
    return nullptr;

#define BRAVE_WEBGL_RENDERING_CONTEXT_BASE_NULLOPT                          \
  if (canvas() && !AllowFingerprinting(canvas()->GetDocument().GetFrame())) \
    return base::nullopt;

#define BRAVE_WEBGL_RENDERING_CONTEXT_BASE_ZERO                             \
  if (canvas() && !AllowFingerprinting(canvas()->GetDocument().GetFrame())) \
    return 0;

#define BRAVE_WEBGL_RENDERING_CONTEXT_BASE_MINUS_ONE                        \
  if (canvas() && !AllowFingerprinting(canvas()->GetDocument().GetFrame())) \
    return -1;

#define BRAVE_WEBGL_RENDERING_CONTEXT_BASE_SCRIPT_VALUE                     \
  if (canvas() && !AllowFingerprinting(canvas()->GetDocument().GetFrame())) \
    return ScriptValue::CreateNull(script_state->GetIsolate());

#define BRAVE_WEBGL_RENDERING_CONTEXT_BASE_STRING                           \
  if (canvas() && !AllowFingerprinting(canvas()->GetDocument().GetFrame())) \
    return String();

#define BRAVE_WEBGL_GET_PARAMETER_UNMASKED_RENDERER                    \
  if (ExtensionEnabled(kWebGLDebugRendererInfoName) && canvas() &&     \
      !AllowFingerprinting(canvas()->GetDocument().GetFrame()))        \
    return WebGLAny(                                                   \
        script_state,                                                  \
        String(brave::BraveSessionCache::From(canvas()->GetDocument()) \
                   .GenerateRandomString("UNMASKED_RENDERER_WEBGL", 8)));

#define BRAVE_WEBGL_GET_PARAMETER_UNMASKED_VENDOR                      \
  if (ExtensionEnabled(kWebGLDebugRendererInfoName) && canvas() &&     \
      !AllowFingerprinting(canvas()->GetDocument().GetFrame()))        \
    return WebGLAny(                                                   \
        script_state,                                                  \
        String(brave::BraveSessionCache::From(canvas()->GetDocument()) \
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
