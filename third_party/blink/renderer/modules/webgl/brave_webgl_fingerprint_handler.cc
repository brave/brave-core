
/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/modules/webgl/brave_webgl_fingerprint_handler.h"

#include <array>
#include <optional>

#include "base/feature_list.h"
#include "base/no_destructor.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/renderer/bindings/core/v8/script_value.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

namespace {

std::array<String, 5> GetFakeExtensionNames() {
  static const base::NoDestructor<std::array<String, 5>> kFakeExtensions(
      {{{"EXT_master_of_puppets"},
        {"EXT_fade_to_black"},
        {"EXT_nothing_else_matters"},
        {"EXT_one"},
        {"EXT_welcome_home_sanitarium"}}});
  return *kFakeExtensions;
}

std::unique_ptr<BraveWebGLFingerprintHandler> CreateOffHandler(
    const ExtensionVector& real_extensions) {
  return std::make_unique<BraveWebGLFingerprintHandler>(real_extensions);
}

std::unique_ptr<BraveWebGLFingerprintHandler> CreateBalancedHandler(
    const ExtensionVector& real_extensions,
    brave::FarblingPRNG prng) {
  if (!base::FeatureList::IsEnabled(
          features::kWebGLBalancedFingerprintingProtection)) {
    return std::make_unique<BraveWebGLFingerprintHandler>(real_extensions);
  }

  const auto& fake_extension_list = GetFakeExtensionNames();

  ExtensionVector modified_extensions = real_extensions;
  const uint8_t fake_index = prng() % fake_extension_list.size();
  modified_extensions.push_back(fake_extension_list[fake_index]);

  return std::make_unique<BraveWebGLFingerprintHandler>(modified_extensions,
                                                        fake_index);
}

std::unique_ptr<BraveWebGLFingerprintHandler> CreateMaximumHandler(
    const ExtensionVector& real_extensions) {
  if (real_extensions.Contains("WEBGL_debug_renderer_info")) {
    return std::make_unique<BraveWebGLFingerprintHandler>(
        ExtensionVector{"WEBGL_debug_renderer_info"});
  }
  return std::make_unique<BraveWebGLFingerprintHandler>(ExtensionVector{});
}

}  // namespace

BraveWebGLFingerprintHandler::BraveWebGLFingerprintHandler(
    const ExtensionVector& supported_extensions,
    std::optional<std::uint8_t> fake_index)
    : supported_extensions_(supported_extensions), fake_index_(fake_index) {}

BraveWebGLFingerprintHandler::~BraveWebGLFingerprintHandler() = default;

ExtensionVector BraveWebGLFingerprintHandler::GetSupportedExtensions() const {
  return supported_extensions_;
}

bool BraveWebGLFingerprintHandler::IsFarbledExtension(
    const String& extension_name) const {
  if (!fake_index_.has_value()) {
    return false;
  }
  const auto& fake_extension_list = GetFakeExtensionNames();
  return fake_extension_list[fake_index_.value()] == extension_name;
}

bool BraveWebGLFingerprintHandler::IsExtensionSupported(
    const String& extension_name) const {
  return supported_extensions_.Contains(extension_name);
}

std::unique_ptr<BraveWebGLFingerprintHandler> CreateWebGLFingerprintHandler(
    const ExtensionVector& real_extensions,
    BraveFarblingLevel farbling_level) {
  switch (farbling_level) {
    case BraveFarblingLevel::OFF:
      return CreateOffHandler(real_extensions);
    case BraveFarblingLevel::MAXIMUM:
      return CreateMaximumHandler(real_extensions);
    case BraveFarblingLevel::BALANCED:
      return CreateBalancedHandler(real_extensions, brave::FarblingPRNG());
    default:
      return CreateOffHandler(real_extensions);
  }
}

}  // namespace blink
