/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/modules/webgl/brave_webgl_fingerprint_handler.h"

#include "base/feature_list.h"
#include "brave/third_party/blink/renderer/brave_farbling_constants.h"
#include "third_party/blink/public/common/features.h"

namespace blink {

class BraveWebGLFingerprintHandlerImpl : public BraveWebGLFingerprintHandler {
 public:
  explicit BraveWebGLFingerprintHandlerImpl(
      const ExtensionVector& supported_extensions)
      : supported_extensions_(supported_extensions) {}

  ~BraveWebGLFingerprintHandlerImpl() override = default;

  ExtensionVector GetSupportedExtensions() const override {
    return supported_extensions_;
  }

  bool IsExtensionSupported(const String& extension_name) const override {
    return supported_extensions_.Contains(extension_name);
  }

 private:
  ExtensionVector supported_extensions_;
};

namespace {

std::unique_ptr<BraveWebGLFingerprintHandler> CreateOffHandler(
    const ExtensionVector& real_extensions) {
  return std::make_unique<BraveWebGLFingerprintHandlerImpl>(real_extensions);
}

std::unique_ptr<BraveWebGLFingerprintHandler> CreateBalancedHandler(
    const ExtensionVector& real_extensions) {
  if (!base::FeatureList::IsEnabled(
          features::kBraveWebGLFingerprintingProtection)) {
    return std::make_unique<BraveWebGLFingerprintHandlerImpl>(real_extensions);
  }

  const ExtensionVector hardcoded_extension_list = {
      "ANGLE_instanced_arrays",
      "EXT_blend_minmax",
      "EXT_color_buffer_half_float",
      "EXT_depth_clamp",
      "EXT_float_blend",
      "EXT_frag_depth",
      "EXT_shader_texture_lod",
      "EXT_sRGB",
      "EXT_texture_compression_rgtc",
      "EXT_texture_filter_anisotropic",
      "OES_element_index_uint",
      "OES_fbo_render_mipmap",
      "OES_standard_derivatives",
      "OES_texture_float",
      "OES_texture_float_linear",
      "OES_texture_half_float",
      "OES_texture_half_float_linear",
      "OES_vertex_array_object",
      "WEBGL_color_buffer_float",
      "WEBGL_compressed_texture_s3tc",
      "WEBGL_compressed_texture_s3tc_srgb",
      "WEBGL_debug_renderer_info",
      "WEBGL_depth_texture",
      "WEBGL_draw_buffers",
      "WEBGL_lose_context",
      "WEBGL_provoking_vertex",
  };

  ExtensionVector supported_hardcoded_extension_list;

  for (const auto& extension : hardcoded_extension_list) {
    if (real_extensions.Contains(extension)) {
      supported_hardcoded_extension_list.push_back(extension);
    }
  }

  return std::make_unique<BraveWebGLFingerprintHandlerImpl>(
      supported_hardcoded_extension_list);
}

std::unique_ptr<BraveWebGLFingerprintHandler> CreateMaximumHandler(
    const ExtensionVector& real_extensions) {
  if (real_extensions.Contains("WEBGL_debug_renderer_info")) {
    return std::make_unique<BraveWebGLFingerprintHandlerImpl>(
        ExtensionVector{"WEBGL_debug_renderer_info"});
  }
  return std::make_unique<BraveWebGLFingerprintHandlerImpl>(ExtensionVector{});
}
}  // namespace

std::unique_ptr<BraveWebGLFingerprintHandler> CreateWebGLFingerprintHandler(
    const ExtensionVector& real_extensions,
    BraveFarblingLevel farbling_level) {
  switch (farbling_level) {
    case BraveFarblingLevel::OFF:
      return CreateOffHandler(real_extensions);
    case BraveFarblingLevel::MAXIMUM:
      return CreateMaximumHandler(real_extensions);
    case BraveFarblingLevel::BALANCED:
      return CreateBalancedHandler(real_extensions);
    default:
      return CreateOffHandler(real_extensions);
  }
}
}  // namespace blink
