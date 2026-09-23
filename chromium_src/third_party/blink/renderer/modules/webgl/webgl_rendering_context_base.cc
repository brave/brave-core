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
#include "third_party/blink/renderer/platform/wtf/text/ascii_ctype.h"

namespace {
const char kUnmaskedVendorWebGL[] = "UNMASKED_VENDOR_WEBGL";
const char kUnmaskedRendererWebGL[] = "UNMASKED_RENDERER_WEBGL";

enum class WebGLDebugRendererInfoType {
  VENDOR,
  RENDERER,
};

bool AllowFingerprintingForHost(blink::CanvasRenderingContextHost* host,
                                const bool is_webgl2) {
  if (!host) {
    return true;
  }
  return brave::AllowFingerprinting(
      host->GetTopExecutionContext(),
      is_webgl2 ? ContentSettingsType::BRAVE_WEBCOMPAT_WEBGL2
                : ContentSettingsType::BRAVE_WEBCOMPAT_WEBGL);
}

blink::ScriptValue GetWebGLDebugInfoValue(
    blink::ScriptState* script_state,
    blink::CanvasRenderingContextHost* host,
    const WebGLDebugRendererInfoType type,
    const blink::String original_string_value,
    bool is_webgl2) {
  auto level = brave::GetBraveFarblingLevelFor(
      host->GetTopExecutionContext(),
      is_webgl2 ? ContentSettingsType::BRAVE_WEBCOMPAT_WEBGL2
                : ContentSettingsType::BRAVE_WEBCOMPAT_WEBGL,
      BraveFarblingLevel::OFF);
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

#define BRAVE_WEBGL_RENDERING_CONTEXT_BASE_RETURN      \
  if (!AllowFingerprintingForHost(Host(), IsWebGL2())) \
    return;

#define BRAVE_WEBGL_RENDERING_CONTEXT_BASE_NULLPTR     \
  if (!AllowFingerprintingForHost(Host(), IsWebGL2())) \
    return nullptr;

#define BRAVE_WEBGL_RENDERING_CONTEXT_BASE_NULLOPT     \
  if (!AllowFingerprintingForHost(Host(), IsWebGL2())) \
    return std::nullopt;

#define BRAVE_WEBGL_RENDERING_CONTEXT_BASE_ZERO        \
  if (!AllowFingerprintingForHost(Host(), IsWebGL2())) \
    return 0;

#define BRAVE_WEBGL_RENDERING_CONTEXT_BASE_MINUS_ONE   \
  if (!AllowFingerprintingForHost(Host(), IsWebGL2())) \
    return -1;

#define BRAVE_WEBGL_RENDERING_CONTEXT_BASE_SCRIPT_VALUE \
  if (!AllowFingerprintingForHost(Host(), IsWebGL2()))  \
    return ScriptValue::CreateNull(v8::Isolate::GetCurrent());

#define BRAVE_WEBGL_RENDERING_CONTEXT_BASE_STRING      \
  if (!AllowFingerprintingForHost(Host(), IsWebGL2())) \
    return String();

#define BRAVE_WEBGL_RENDERING_CONTEXT_BASE_GETSHADERINFOLOG \
  if (!AllowFingerprintingForHost(Host(), IsWebGL2())) {    \
    range[0] = 0;                                           \
    range[1] = 0;                                           \
    precision = 0;                                          \
  }

#define BRAVE_WEBGL_GET_PARAMETER_UNMASKED_RENDERER                 \
  if (ExtensionEnabled(kWebGLDebugRendererInfoName))                \
    return GetWebGLDebugInfoValue(                                  \
        script_state, Host(), WebGLDebugRendererInfoType::RENDERER, \
        String(ContextGL()->GetString(GL_RENDERER)), IsWebGL2());

#define BRAVE_WEBGL_GET_PARAMETER_UNMASKED_VENDOR                 \
  if (ExtensionEnabled(kWebGLDebugRendererInfoName))              \
    return GetWebGLDebugInfoValue(                                \
        script_state, Host(), WebGLDebugRendererInfoType::VENDOR, \
        String(ContextGL()->GetString(GL_VENDOR)), IsWebGL2());

#define getExtension getExtension_ChromiumImpl
#define getSupportedExtensions getSupportedExtensions_ChromiumImpl
#include <third_party/blink/renderer/modules/webgl/webgl_rendering_context_base.cc>
#undef getSupportedExtensions
#undef getExtension

namespace blink {

namespace {

// An opaque method to get a valid WebGL extension handler. If the handler
// does not exist it will create a new one.
WebGLFarbledExtensionHandler* CreateOrGetFarblingExtensionHandler(
    ExecutionContext* context,
    bool is_webgl2) {
  // Check if we have a valid handler for the current context.
  auto& cache = brave::BraveSessionCache::From(*context);
  WebGLFarbledExtensionHandler* handler =
      cache.get_webgl_farbled_extension_handler(is_webgl2);

  // No valid handler found so create a new one which will be re-used for this
  // WebGL API version until the lifetime of the execution context.
  if (!handler) {
    handler = cache.CreateWebGLFarbledExtensionHandler(is_webgl2);
  }
  return handler;
}

}  // namespace

// This method returns the supported WebGL/WebGL2 extensions. If fingerprinting
// protections are enabled then the list may include farbled values.
std::optional<Vector<String>>
WebGLRenderingContextBase::getSupportedExtensions() {
  std::optional<Vector<String>> real_extensions =
      getSupportedExtensions_ChromiumImpl();
  if (real_extensions == std::nullopt) {
    return real_extensions;
  }

  const auto level =
      Host() ? brave::GetBraveFarblingLevelFor(
                   Host()->GetTopExecutionContext(),
                   IsWebGL2() ? ContentSettingsType::BRAVE_WEBCOMPAT_WEBGL2
                              : ContentSettingsType::BRAVE_WEBCOMPAT_WEBGL,
                   BraveFarblingLevel::OFF)
             : BraveFarblingLevel::OFF;

  // Balanced case + feature flag: Farble the extension set.
  if (level == BraveFarblingLevel::BALANCED &&
      base::FeatureList::IsEnabled(
          features::kWebGLBalancedFingerprintingProtection)) {
    WebGLFarbledExtensionHandler* handler = CreateOrGetFarblingExtensionHandler(
        Host()->GetTopExecutionContext(), IsWebGL2());
    real_extensions.value().push_back(handler->GetExtensionName());
    return real_extensions;
  }

  // TODO(https://github.com/brave/brave-browser/issues/57897): Remove this once
  // the strict fingerprinting mode is removed.
  if (level == BraveFarblingLevel::MAXIMUM) {
    Vector<String> fake_extensions;
    fake_extensions.push_back(WebGLDebugRendererInfo::ExtensionName());
    return fake_extensions;
  }

  // For all other cases we return the original list of extensions.
  return real_extensions;
}

// This method return the underlying extension ScriptObject for the given
// extension |name|. The returned ScriptObject may hold a null value if
// the |name| does not correspond to the list of supported extensions. It could
// also represent a farbled object if the extension |name| was farbled.
ScriptObject WebGLRenderingContextBase::getExtension(ScriptState* script_state,
                                                     const String& name) {
  const auto level =
      Host() ? brave::GetBraveFarblingLevelFor(
                   Host()->GetTopExecutionContext(),
                   IsWebGL2() ? ContentSettingsType::BRAVE_WEBCOMPAT_WEBGL2
                              : ContentSettingsType::BRAVE_WEBCOMPAT_WEBGL,
                   BraveFarblingLevel::OFF)
             : BraveFarblingLevel::OFF;

  // TODO(https://github.com/brave/brave-browser/issues/57897): Remove this once
  // the strict fingerprinting mode is removed.
  if (level == BraveFarblingLevel::MAXIMUM) {
    return (name != WebGLDebugRendererInfo::ExtensionName())
               ? ScriptObject::CreateNull(v8::Isolate::GetCurrent())
               : getExtension_ChromiumImpl(script_state, name);
  }

  // Special case if the |name| was farbled.
  if (level == BraveFarblingLevel::BALANCED &&
      base::FeatureList::IsEnabled(
          features::kWebGLBalancedFingerprintingProtection)) {
    WebGLFarbledExtensionHandler* handler = CreateOrGetFarblingExtensionHandler(
        Host()->GetTopExecutionContext(), IsWebGL2());
    // Client is asking for the farbled script value. EqualIgnoringAsciiCase to
    // ensure we handle cases where client can pass a case-insensitive name.
    // See https://github.com/brave/brave-browser/issues/57902.
    if (EqualIgnoringAsciiCase(handler->GetExtensionName(), name)) {
      // Time to build up a fake object.
      v8::Isolate* isolate = script_state->GetIsolate();
      v8::Local<v8::Context> context = script_state->GetContext();
      v8::Local<v8::FunctionTemplate> tmpl = v8::FunctionTemplate::New(isolate);
      tmpl->SetClassName(V8String(isolate, handler->GetExtensionObjectName()));
      tmpl->PrototypeTemplate()->Set(
          v8::Symbol::GetToStringTag(isolate),
          V8String(isolate, handler->GetExtensionObjectName()),
          static_cast<v8::PropertyAttribute>(v8::ReadOnly | v8::DontEnum));
      v8::Local<v8::Object> obj = tmpl->GetFunction(context)
                                      .ToLocalChecked()
                                      ->NewInstance(context)
                                      .ToLocalChecked();
      return blink::ScriptObject(isolate, obj);
    }
  }

  // Upstream would return a null Script object for un-defined names.
  return getExtension_ChromiumImpl(script_state, name);
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
