/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/renderer/brave_content_settings_agent_impl_helper.h"
#include "third_party/blink/renderer/core/dom/document.h"

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

#include "../../../../../../third_party/blink/renderer/modules/webgl/webgl_rendering_context_base.cc"  // NOLINT

#undef BRAVE_WEBGL_GET_PARAMETER_UNMASKED_RENDERER
#undef BRAVE_WEBGL_GET_PARAMETER_UNMASKED_VENDOR
