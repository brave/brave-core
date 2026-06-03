/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/modules/webgl/webgl_rendering_context_base.h"

#include <optional>

#include "brave/third_party/blink/renderer/bindings/core/webgl/webgl_farbled_extension_handler.h"
#include "brave/third_party/blink/renderer/core/farbling/brave_session_cache.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/renderer/bindings/modules/v8/webgl_any.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/html/canvas/canvas_rendering_context_host.h"
#include "third_party/blink/renderer/core/workers/worker_global_scope.h"

namespace {
const char kUnmaskedVendorWebGL[] = "UNMASKED_VENDOR_WEBGL";
const char kUnmaskedRendererWebGL[] = "UNMASKED_RENDERER_WEBGL";

enum class WebGLDebugRendererInfoType {
  VENDOR,
  RENDERER,
};

bool AllowFingerprintingForHost(blink::CanvasRenderingContextHost* host) {
  if (!host) {
    return true;
  }
  return brave::AllowFingerprinting(host->GetTopExecutionContext(),
                                    ContentSettingsType::BRAVE_WEBCOMPAT_WEBGL);
}

blink::ScriptValue GetWebGLDebugInfoValue(
    blink::ScriptState* script_state,
    blink::CanvasRenderingContextHost* host,
    const WebGLDebugRendererInfoType type,
    const blink::String original_string_value) {
  auto level = brave::GetBraveFarblingLevelFor(
      host->GetTopExecutionContext(),
      ContentSettingsType::BRAVE_WEBCOMPAT_WEBGL, BraveFarblingLevel::OFF);
  blink::ScriptValue original_script_value =
      blink::WebGLAny(script_state, original_string_value);

  switch (level) {
    case BraveFarblingLevel::OFF:
      return original_script_value;
    case BraveFarblingLevel::BALANCED:
      return base::FeatureList::IsEnabled(
                 blink::features::kWebGLBalancedFingerprintingProtection)
                 ? blink::WebGLAny(script_state, blink::String("Brave"))
                 : original_script_value;
    case BraveFarblingLevel::MAXIMUM:
      return blink::WebGLAny(
          script_state,
          blink::String(
              brave::BraveSessionCache::From(*(host->GetTopExecutionContext()))
                  .GenerateRandomString(
                      type == WebGLDebugRendererInfoType::VENDOR
                          ? kUnmaskedVendorWebGL
                          : kUnmaskedRendererWebGL,
                      8)));
    default:
      return original_script_value;
  }
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
    return std::nullopt;

#define BRAVE_WEBGL_RENDERING_CONTEXT_BASE_ZERO \
  if (!AllowFingerprintingForHost(Host()))      \
    return 0;

#define BRAVE_WEBGL_RENDERING_CONTEXT_BASE_MINUS_ONE \
  if (!AllowFingerprintingForHost(Host()))           \
    return -1;

#define BRAVE_WEBGL_RENDERING_CONTEXT_BASE_SCRIPT_VALUE \
  if (!AllowFingerprintingForHost(Host()))              \
    return ScriptValue::CreateNull(v8::Isolate::GetCurrent());

#define BRAVE_WEBGL_RENDERING_CONTEXT_BASE_STRING \
  if (!AllowFingerprintingForHost(Host()))        \
    return String();

#define BRAVE_WEBGL_RENDERING_CONTEXT_BASE_GETSHADERINFOLOG \
  if (!AllowFingerprintingForHost(Host())) {                \
    range[0] = 0;                                           \
    range[1] = 0;                                           \
    precision = 0;                                          \
  }

#define BRAVE_WEBGL_GET_PARAMETER_UNMASKED_RENDERER                 \
  if (ExtensionEnabled(kWebGLDebugRendererInfoName))                \
    return GetWebGLDebugInfoValue(                                  \
        script_state, Host(), WebGLDebugRendererInfoType::RENDERER, \
        String(ContextGL()->GetString(GL_RENDERER)));

#define BRAVE_WEBGL_GET_PARAMETER_UNMASKED_VENDOR                     \
  if (ExtensionEnabled(kWebGLDebugRendererInfoName))                  \
    return GetWebGLDebugInfoValue(script_state, Host(),               \
                                  WebGLDebugRendererInfoType::VENDOR, \
                                  String(ContextGL()->GetString(GL_VENDOR)));

#define getExtension getExtension_ChromiumImpl
#define getSupportedExtensions getSupportedExtensions_ChromiumImpl
#include <third_party/blink/renderer/modules/webgl/webgl_rendering_context_base.cc>
#undef getSupportedExtensions
#undef getExtension

namespace blink {

namespace {

// An opaque method to get a valid WebGL extension handler. If the handler
// does not exist it will create a new one.
// |get_real_extensions| is a lambda which when invoked returns the true list of
// supported extensions. The true list is needed to create the appropriate
// handler.
template <typename T>
blink::WebGLFarbledExtensionHandler* GetValidHandler(ExecutionContext* context,
                                                     T&& get_real_extensions) {
  // Check if we have a valid handler for the current context.
  WebGLFarbledExtensionHandler* handler =
      brave::BraveSessionCache::From(*context)
          .get_webgl_farbled_extension_handler();

  // No valid handler found so create a new one which will be re-used until the
  // lifetime of this context.
  if (!handler) {
    // Get the real list of supported WebGL extensions.
    std::optional<Vector<String>> real_extensions =
        std::forward<T>(get_real_extensions)();
    if (real_extensions == std::nullopt) {
      return nullptr;
    }
    handler = brave::BraveSessionCache::From(*context)
                  .GetWebGLFarbledExtensionHandler(real_extensions.value());
  }
  return handler;
}

}  // namespace

// This method returns the supported WebGL extensions. The returned list of
// extensions may not correspond to the real ones if fingerprinting
// protections are enabled.
std::optional<Vector<String>>
WebGLRenderingContextBase::getSupportedExtensions() {
  // Check if we have a valid handler for the current context.
  WebGLFarbledExtensionHandler* handler = GetValidHandler(
      Host()->GetTopExecutionContext(),
      [this]() { return getSupportedExtensions_ChromiumImpl(); });

  // Handler can be null when if there were no supported extensions found.
  if (!handler) {
    return std::nullopt;
  }

  // Return the final list of WebGL extensions which may have been farbled.
  return handler->GetSupportedExtensions();
}

// This method return the underlying extension ScriptObject for the given
// extension |name|. The returned ScriptObject may hold a null value if
// the |name| does not correspond to the list of supported extensions. It may
// also hold a dummy value if the |name| was farbled.
ScriptObject WebGLRenderingContextBase::getExtension(ScriptState* script_state,
                                                     const String& name) {
  // Check if we have a valid handler for the current context.
  WebGLFarbledExtensionHandler* handler = GetValidHandler(
      Host()->GetTopExecutionContext(),
      [this]() { return getSupportedExtensions_ChromiumImpl(); });

  if (!handler) {
    return ScriptObject::CreateNull(v8::Isolate::GetCurrent());
  }

  // Upstream takes care of returning nullable ScriptObject for invalid names.
  const ScriptObject real_extension =
      getExtension_ChromiumImpl(script_state, name);
  // Handler would apply farbling on valid extension name. If not valid it would
  // return real_extension.
  return handler->GetExtension(script_state, name, &real_extension);
}

}  // namespace blink

#undef BRAVE_WEBGL_GET_PARAMETER_UNMASKED_VENDOR
#undef BRAVE_WEBGL_GET_PARAMETER_UNMASKED_RENDERER
#undef BRAVE_WEBGL_RENDERING_CONTEXT_BASE_GETSHADERINFOLOG
#undef BRAVE_WEBGL_RENDERING_CONTEXT_BASE_STRING
#undef BRAVE_WEBGL_RENDERING_CONTEXT_BASE_SCRIPT_VALUE
#undef BRAVE_WEBGL_RENDERING_CONTEXT_BASE_MINUS_ONE
#undef BRAVE_WEBGL_RENDERING_CONTEXT_BASE_ZERO
#undef BRAVE_WEBGL_RENDERING_CONTEXT_BASE_NULLOPT
#undef BRAVE_WEBGL_RENDERING_CONTEXT_BASE_NULLPTR
#undef BRAVE_WEBGL_RENDERING_CONTEXT_BASE_RETURN
