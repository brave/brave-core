/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_WEBGL_WEBGL_FARBLED_EXTENSION_HANDLER_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_WEBGL_WEBGL_FARBLED_EXTENSION_HANDLER_H_

#include <base/containers/span.h>

#include <cstdint>
#include <memory>

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

// Internal data type of the fake WebGL extensions array.
struct WebGLFakeExtension {
  // The real name of the fake extension returned by the getSupportedExtension
  // call when farbled. For example: EXT_texture_sampler.
  String name;
  // The underlying name of the script object which is returned by the
  // getExtension call when farbled. For example: ExtTextureSampler
  String script_object_name;
};

// Handler for returning information around the available webgl extensions.
// This handler automatically takes into consideration any farbling when the
// brave shields are up.
class CORE_EXPORT WebGLFarbledExtensionHandler {
 public:
  // Returns a handler with default fingerprinting protections.
  // |seed| is derived from the current brave::FarblingToken for the session.
  static std::unique_ptr<WebGLFarbledExtensionHandler> CreateHandler(
      const uint64_t seed);

  ~WebGLFarbledExtensionHandler();

  // Returns the farbled extension name as seen by getSupportedExtensions.
  String GetExtensionName() const;

  // Returns the farbled extension object name as seen by getExtension.
  String GetExtensionObjectName() const;

 private:
  explicit WebGLFarbledExtensionHandler(WebGLFakeExtension fake_extension);

  // The fake extension.
  const WebGLFakeExtension fake_extension_;
};

// A test only method to fetch the list of fake extensions.
CORE_EXPORT base::span<const WebGLFakeExtension>
GetFakeSupportedExtensionsForTesting();
}  // namespace blink

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_WEBGL_WEBGL_FARBLED_EXTENSION_HANDLER_H_
