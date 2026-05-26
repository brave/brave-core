/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/bindings/core/webgl/webgl_extension_handler.h"

#include <array>
#include <optional>

#include "base/feature_list.h"
#include "base/no_destructor.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/renderer/bindings/core/v8/script_value.h"
#include "third_party/blink/renderer/platform/bindings/v8_binding.h"
#include "third_party/blink/renderer/platform/wtf/text/ascii_ctype.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

namespace {

std::array<String, 21> GetFakeSupportedExtensions() {
  // The entries in the list does not actually exists as a supported extension
  // in WebGL. The below names are inspired from existing names (see
  // https://registry.khronos.org/webgl/extensions/) to make the list a little
  // more palapable if any sanity check is ever done by a human reviewer. Though
  // this is not full-proof but the core idea here is to defuse any
  // automated fingerprinting scripts that relies on a hash based approach.
  // Through farbling, we will inject one of the fake values below to the
  // result of the getSupportedExtensions call which would be consistent across
  // that eTLD+1 session. A different session would psuedo-randomly yield a
  // different list resulting in a different hash.
  //
  // The list below is a flat representation of the two sets {texture, expanded,
  // polygon, circle, triangle, blend, draw} and {sampler, blender, compressor}
  // which does not exists.
  static const base::NoDestructor<std::array<String, 21>> fake_values({{
      {"EXT_texture_sampler"},     {"EXT_texture_compressor"},
      {"EXT_texture_blender"},     {"EXT_expanded_sampler"},
      {"EXT_expanded_compressor"}, {"EXT_expanded_blender"},
      {"EXT_polygon_sampler"},     {"EXT_polygon_compressor"},
      {"EXT_polygon_blender"},     {"EXT_circle_sampler"},
      {"EXT_circle_compressor"},   {"EXT_circle_blender"},
      {"EXT_triangle_sampler"},    {"EXT_triangle_compressor"},
      {"EXT_triangle_blender"},    {"EXT_blend_sampler"},
      {"EXT_blend_compressor"},    {"EXT_blend_blender"},
      {"EXT_draw_sampler"},        {"EXT_draw_compressor"},
      {"EXT_draw_blender"},
  }});
  return *fake_values;
}

std::unique_ptr<WebGLExtensionHandler> CreateBalancedHandler(
    const ExtensionVector& real_extensions,
    uint64_t seed) {
  if (!base::FeatureList::IsEnabled(
          features::kWebGLBalancedFingerprintingProtection)) {
    return std::make_unique<WebGLExtensionHandler>(real_extensions);
  }

  ExtensionVector modified_extensions = real_extensions;
  const auto& fake_extension_list = GetFakeSupportedExtensions();
  const uint8_t fake_index = seed % fake_extension_list.size();
  modified_extensions.push_back(fake_extension_list[fake_index]);

  // The fake_index is now stable across the lifetime of the current
  // eTLD+1 session. The index would be reset when the owner destroys the
  // handler.
  return std::make_unique<WebGLExtensionHandler>(modified_extensions,
                                                 fake_index);
}

std::unique_ptr<WebGLExtensionHandler> CreateMaximumHandler(
    const ExtensionVector& real_extensions) {
  if (real_extensions.Contains("WEBGL_debug_renderer_info")) {
    return std::make_unique<WebGLExtensionHandler>(
        ExtensionVector{"WEBGL_debug_renderer_info"});
  }
  return std::make_unique<WebGLExtensionHandler>(ExtensionVector{});
}

// Converts an extension name like "EXT_draw_compressor" to a CamelCase class
// name like "ExtDrawCompressor" by title-casing each underscore-delimited
// segment.
String ToExtensionClassName(const String& name) {
  StringBuilder result;
  bool capitalize_next = true;
  for (unsigned i = 0; i < name.length(); ++i) {
    const UChar c = name[i];
    if (c == '_') {
      capitalize_next = true;
    } else if (capitalize_next) {
      result.Append(static_cast<UChar>(ToAsciiUpper(c)));
      capitalize_next = false;
    } else {
      result.Append(static_cast<UChar>(ToAsciiLower(c)));
    }
  }
  return result.ToString();
}

}  // namespace

WebGLExtensionHandler::WebGLExtensionHandler(
    const ExtensionVector& supported_extensions,
    std::optional<std::uint8_t> fake_index)
    : supported_extensions_(supported_extensions), fake_index_(fake_index) {}

WebGLExtensionHandler::~WebGLExtensionHandler() = default;

ExtensionVector WebGLExtensionHandler::GetSupportedExtensions() const {
  return supported_extensions_;
}

bool WebGLExtensionHandler::IsExtensionFarbled(const String& name) const {
  if (!fake_index_.has_value()) {
    return false;
  }
  const auto& fake_extension_list = GetFakeSupportedExtensions();
  return fake_extension_list[fake_index_.value()] == name;
}

ScriptObject WebGLExtensionHandler::GetExtension(
    ScriptState* script_state,
    const String& name,
    const ScriptObject* real_extension) {
  // Create and return a fake object if the extension was farbled.
  if (IsExtensionFarbled(name)) {
    v8::Isolate* isolate = script_state->GetIsolate();
    v8::Local<v8::Context> context = script_state->GetContext();
    v8::Local<v8::FunctionTemplate> tmpl = v8::FunctionTemplate::New(isolate);
    tmpl->SetClassName(V8String(isolate, ToExtensionClassName(name)));
    v8::Local<v8::Object> obj = tmpl->GetFunction(context)
                                    .ToLocalChecked()
                                    ->NewInstance(context)
                                    .ToLocalChecked();
    return ScriptObject(isolate, obj);
  }

  // If extension is part of the supported list return the real extension.
  if (supported_extensions_.Contains(name)) {
    return *real_extension;
  }

  return ScriptObject::CreateNull(v8::Isolate::GetCurrent());
}

std::unique_ptr<WebGLExtensionHandler> CreateOffHandler(
    const ExtensionVector& real_extensions) {
  return std::make_unique<WebGLExtensionHandler>(real_extensions);
}

std::unique_ptr<WebGLExtensionHandler> CreateFarblingHandler(
    const ExtensionVector& real_extensions,
    const bool create_maximum_handler,
    const uint64_t seed) {
  return create_maximum_handler ? CreateMaximumHandler(real_extensions)
                                : CreateBalancedHandler(real_extensions, seed);
}

}  // namespace blink
