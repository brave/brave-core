/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_WEBGL_WEBGL_EXTENSION_HANDLER_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_WEBGL_WEBGL_EXTENSION_HANDLER_H_

#include <base/containers/span.h>

#include <memory>
#include <optional>

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {
class ScriptObject;
class ScriptState;
}  // namespace blink

namespace webgl {

using ExtensionVector = blink::Vector<blink::String>;

// Internal data type of the fake WebGL extensions array.
struct FakeExtensionType {
  // The real name of the fake extension returned by the getSupportedExtension
  // call when farbled. For example: EXT_texture_sampler.
  blink::String name;
  // The underlying name of the script object which is returned by the
  // getExtension call when farbled.
  blink::String script_object_name;
};

// Handler for returning information around the available webgl extensions.
// This handler automatically takes into consideration any farbling when the
// brave shields are up.
// TODO(https://github.com/brave/brave-browser/issues/55858): Add the
// integration with the BraveSessionCache.
class CORE_EXPORT WebGLExtensionHandler {
 public:
  WebGLExtensionHandler(
      const ExtensionVector& supported_extensions,
      std::optional<FakeExtensionType> fake_extension = std::nullopt);
  ~WebGLExtensionHandler();

  // Returns a vector of supported extensions which could also include a farbled
  // extension.
  ExtensionVector GetSupportedExtensions() const;

  // Returns a ScriptObject which is either |real_extension| or a
  // dummy ScriptObject with the internal value set to the farbled extension
  // |name|. If |name| doesn't exists in |supported_extensions_| then it returns
  // an ScriptObject with null value. |real_extension| must be non-null when
  // |name| is in |supported_extensions_|.
  blink::ScriptObject GetExtension(blink::ScriptState* script_state,
                                   const blink::String& name,
                                   const blink::ScriptObject* real_extension);

 private:
  // Returns true if |extension_name| is a farbled value, false otherwise.
  bool IsExtensionFarbled(const blink::String& extension_name) const;

  // The list of the supported webgl extensions including any farbled extension.
  const ExtensionVector supported_extensions_;
  // The fake extension.
  std::optional<FakeExtensionType> fake_extension_;
};

// Returns the default handler without any farbling logic.
// |real_extensions| is the set of the actual supported WebGL extensions by the
// machine.
CORE_EXPORT std::unique_ptr<WebGLExtensionHandler> CreateOffHandler(
    const ExtensionVector& real_extensions);

// Returns a handler with default fingerprinting protections.
// |real_extensions| is the set of actual supported WebGL extensions.
// |seed| is derived from the current brave::FarblingToken for the session.
CORE_EXPORT std::unique_ptr<WebGLExtensionHandler> CreateBalancedHandler(
    const ExtensionVector& real_extensions,
    const size_t seed);

// Return a handler which maximum fingerprinting protections.
// |real_extensions| is the set of actual supported WebGL extensions.
CORE_EXPORT std::unique_ptr<WebGLExtensionHandler> CreateMaximumHandler(
    const ExtensionVector& real_extensions);

// A test only method to fetch the list of fake extensions.
CORE_EXPORT base::span<const FakeExtensionType>
GetFakeSupportedExtensionsForTesting();
}  // namespace webgl

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_WEBGL_WEBGL_EXTENSION_HANDLER_H_
