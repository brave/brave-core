/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/bindings/core/webgl/webgl_farbled_extension_handler.h"

#include <array>
#include <optional>

#include "base/check.h"
#include "base/check_is_test.h"
#include "base/feature_list.h"
#include "base/no_destructor.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/renderer/bindings/core/v8/script_value.h"
#include "third_party/blink/renderer/platform/bindings/v8_binding.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

namespace {
// The size of the fake extension list.
inline constexpr size_t kFakeExtensionsSize = 21;

// The following *fake* extension names are inspired from existing names (see
// https://registry.khronos.org/webgl/extensions/). This fake list serves to
// defuse any automated fingerprinting scripts that relies on a hash based
// approach. Through farbling, we will inject one of the following fake values
// to the result of the getSupportedExtensions call which would be consistent
// across that eTLD+1 session. A different session would pseudo-randomly yield a
// different list resulting in a different hash. Hence the two call sites can't
// link it's the same user via the hash based approach.
//
// The list below is a flat representation of the two sets {texture, expanded,
// polygon, circle, triangle, blend, draw} and {sampler, blender, compressor}.
const std::array<WebGLFakeExtension, kFakeExtensionsSize>&
GetFakeSupportedExtensions() {
  static const base::NoDestructor<
      std::array<WebGLFakeExtension, kFakeExtensionsSize>>
      fake_values({{
          {"EXT_texture_sampler", "ExtTextureSampler"},
          {"EXT_texture_compressor", "ExtTextureCompressor"},
          {"EXT_texture_blender", "ExtTextureBlender"},
          {"EXT_expanded_sampler", "ExtExpandedSampler"},
          {"EXT_expanded_compressor", "ExtExpandedCompressor"},
          {"EXT_expanded_blender", "ExtExpandedBlender"},
          {"EXT_polygon_sampler", "ExtPolygonSampler"},
          {"EXT_polygon_compressor", "ExtPolygonCompressor"},
          {"EXT_polygon_blender", "ExtPolygonBlender"},
          {"EXT_circle_sampler", "ExtCircleSampler"},
          {"EXT_circle_compressor", "ExtCircleCompressor"},
          {"EXT_circle_blender", "ExtCircleBlender"},
          {"EXT_triangle_sampler", "ExtTriangleSampler"},
          {"EXT_triangle_compressor", "ExtTriangleCompressor"},
          {"EXT_triangle_blender", "ExtTriangleBlender"},
          {"EXT_blend_sampler", "ExtBlendSampler"},
          {"EXT_blend_compressor", "ExtBlendCompressor"},
          {"EXT_blend_blender", "ExtBlendBlender"},
          {"EXT_draw_sampler", "ExtDrawSampler"},
          {"EXT_draw_compressor", "ExtDrawCompressor"},
          {"EXT_draw_blender", "ExtDrawBlender"},
      }});
  return *fake_values;
}

}  // namespace

WebGLFarbledExtensionHandler::WebGLFarbledExtensionHandler(
    const Vector<String>& supported_extensions,
    std::optional<WebGLFakeExtension> fake_extension)
    : supported_extensions_(supported_extensions),
      fake_extension_(fake_extension) {}

WebGLFarbledExtensionHandler::~WebGLFarbledExtensionHandler() = default;

// static
std::unique_ptr<WebGLFarbledExtensionHandler>
WebGLFarbledExtensionHandler::CreateOffHandler(
    const Vector<String>& real_extensions) {
  return std::unique_ptr<WebGLFarbledExtensionHandler>(
      new WebGLFarbledExtensionHandler(real_extensions));
}

// static
std::unique_ptr<WebGLFarbledExtensionHandler>
WebGLFarbledExtensionHandler::CreateBalancedHandler(
    const Vector<String>& real_extensions,
    const size_t seed) {
  if (!base::FeatureList::IsEnabled(
          blink::features::kWebGLBalancedFingerprintingProtection)) {
    return std::unique_ptr<WebGLFarbledExtensionHandler>(
        new WebGLFarbledExtensionHandler(real_extensions));
  }

  Vector<String> modified_extensions = real_extensions;
  const auto& fake_extension_list = GetFakeSupportedExtensions();
  const size_t fake_index = seed % fake_extension_list.size();
  const WebGLFakeExtension& fake_extension = fake_extension_list[fake_index];
  modified_extensions.emplace_back(fake_extension.name);

  // The fake_extension_name is now stable until the lifetime of the handler.
  return std::unique_ptr<WebGLFarbledExtensionHandler>(
      new WebGLFarbledExtensionHandler(modified_extensions, fake_extension));
}

// static
std::unique_ptr<WebGLFarbledExtensionHandler>
WebGLFarbledExtensionHandler::CreateMaximumHandler(
    const Vector<String>& real_extensions) {
  if (real_extensions.Contains("WEBGL_debug_renderer_info")) {
    return std::unique_ptr<WebGLFarbledExtensionHandler>(
        new WebGLFarbledExtensionHandler(
            Vector<String>{"WEBGL_debug_renderer_info"}));
  }
  return std::unique_ptr<WebGLFarbledExtensionHandler>(
      new WebGLFarbledExtensionHandler(Vector<String>{}));
}

Vector<String> WebGLFarbledExtensionHandler::GetSupportedExtensions() const {
  return supported_extensions_;
}

// TODO(https://github.com/brave/brave-browser/issues/55858): Cover testing this
// in browser_tests as it's not straightforward to test it as unittest due to
// various v8 dependencies.
blink::ScriptObject WebGLFarbledExtensionHandler::GetExtension(
    blink::ScriptState* script_state,
    const blink::String& name,
    const blink::ScriptObject* real_extension) {
  // Create and return a fake object if the extension was farbled.
  if (IsExtensionFarbled(name)) {
    v8::Isolate* isolate = script_state->GetIsolate();
    v8::Local<v8::Context> context = script_state->GetContext();
    v8::Local<v8::FunctionTemplate> tmpl = v8::FunctionTemplate::New(isolate);
    tmpl->SetClassName(V8String(isolate, fake_extension_->script_object_name));
    v8::Local<v8::Object> obj = tmpl->GetFunction(context)
                                    .ToLocalChecked()
                                    ->NewInstance(context)
                                    .ToLocalChecked();
    return blink::ScriptObject(isolate, obj);
  }

  // If extension is part of the supported list return the real extension.
  if (supported_extensions_.Contains(name)) {
    CHECK(real_extension);
    return *real_extension;
  }

  return blink::ScriptObject::CreateNull(script_state->GetIsolate());
}

bool WebGLFarbledExtensionHandler::IsExtensionFarbled(
    const blink::String& name) const {
  return fake_extension_.has_value() && fake_extension_.value().name == name;
}

base::span<const WebGLFakeExtension> GetFakeSupportedExtensionsForTesting() {
  CHECK_IS_TEST();
  return base::span(GetFakeSupportedExtensions());
}
}  // namespace blink
