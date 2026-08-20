/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/bindings/core/webgl/webgl_farbled_extension_handler.h"

#include <array>

#include "base/check.h"
#include "base/check_is_test.h"
#include "base/feature_list.h"
#include "base/no_destructor.h"
#include "third_party/blink/public/common/features.h"

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
    WebGLFakeExtension fake_extension)
    : fake_extension_(fake_extension) {}

WebGLFarbledExtensionHandler::~WebGLFarbledExtensionHandler() = default;

// static
std::unique_ptr<WebGLFarbledExtensionHandler>
WebGLFarbledExtensionHandler::CreateHandler(const uint64_t seed) {
  DCHECK(base::FeatureList::IsEnabled(
      blink::features::kWebGLBalancedFingerprintingProtection));

  const auto& fake_extension_list = GetFakeSupportedExtensions();
  const size_t fake_index =
      static_cast<size_t>(seed % fake_extension_list.size());

  // The fake_extension_name is now stable until the lifetime of the handler.
  return std::unique_ptr<WebGLFarbledExtensionHandler>(
      new WebGLFarbledExtensionHandler(fake_extension_list[fake_index]));
}

String WebGLFarbledExtensionHandler::GetExtensionName() const {
  return fake_extension_.name;
}

String WebGLFarbledExtensionHandler::GetExtensionObjectName() const {
  return fake_extension_.script_object_name;
}

base::span<const WebGLFakeExtension> GetFakeSupportedExtensionsForTesting() {
  CHECK_IS_TEST();
  return base::span(GetFakeSupportedExtensions());
}
}  // namespace blink
