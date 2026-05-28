/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_WEBGL_WEBGL_FARBLED_EXTENSION_HANDLER_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_WEBGL_WEBGL_FARBLED_EXTENSION_HANDLER_H_

#include <base/containers/span.h>

#include <memory>
#include <optional>

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {
class ScriptObject;
class ScriptState;

// Internal data type of the fake WebGL extensions array.
struct WebGLFakeExtension {
  // The real name of the fake extension returned by the getSupportedExtension
  // call when farbled. For example: EXT_texture_sampler.
  String name;
  // The underlying name of the script object which is returned by the
  // getExtension call when farbled.
  String script_object_name;
};

// Handler for returning information around the available webgl extensions.
// This handler automatically takes into consideration any farbling when the
// brave shields are up.
// TODO(https://github.com/brave/brave-browser/issues/55858): Add the
// integration with the BraveSessionCache.
class CORE_EXPORT WebGLFarbledExtensionHandler {
 public:
  // Returns the default handler without any farbling logic.
  // |real_extensions| is the set of the actual supported WebGL extensions by
  // the machine.
  static std::unique_ptr<WebGLFarbledExtensionHandler> CreateOffHandler(
      const Vector<String>& real_extensions);

  // Returns a handler with default fingerprinting protections.
  // |real_extensions| is the set of actual supported WebGL extensions.
  // |seed| is derived from the current brave::FarblingToken for the session.
  static std::unique_ptr<WebGLFarbledExtensionHandler> CreateBalancedHandler(
      const Vector<String>& real_extensions,
      const size_t seed);

  // Return a handler which maximum fingerprinting protections.
  // |real_extensions| is the set of actual supported WebGL extensions.
  static std::unique_ptr<WebGLFarbledExtensionHandler> CreateMaximumHandler(
      const Vector<String>& real_extensions);

  ~WebGLFarbledExtensionHandler();

  // Returns a vector of supported extensions which could also include a farbled
  // extension.
  Vector<String> GetSupportedExtensions() const;

  // Returns a ScriptObject which is either |real_extension| or a
  // dummy ScriptObject with the internal value set to the farbled extension
  // |name|. If |name| doesn't exists in |supported_extensions_| then it returns
  // an ScriptObject with null value. |real_extension| must be non-null when
  // |name| is in |supported_extensions_|.
  ScriptObject GetExtension(ScriptState* script_state,
                            const String& name,
                            const ScriptObject* real_extension);

 private:
  WebGLFarbledExtensionHandler(
      const Vector<String>& supported_extensions,
      std::optional<WebGLFakeExtension> fake_extension = std::nullopt);

  // Returns true if |extension_name| is a farbled value, false otherwise.
  bool IsExtensionFarbled(const String& extension_name) const;

  // The list of the supported webgl extensions including any farbled extension.
  const Vector<String> supported_extensions_;
  // The fake extension.
  std::optional<WebGLFakeExtension> fake_extension_;
};

// A test only method to fetch the list of fake extensions.
CORE_EXPORT base::span<const WebGLFakeExtension>
GetFakeSupportedExtensionsForTesting();
}  // namespace blink

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_WEBGL_WEBGL_FARBLED_EXTENSION_HANDLER_H_
