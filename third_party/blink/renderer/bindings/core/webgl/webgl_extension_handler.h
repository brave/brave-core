/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_WEBGL_WEBGL_EXTENSION_HANDLER_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_WEBGL_WEBGL_EXTENSION_HANDLER_H_

#include <memory>
#include <optional>

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

class String;
class ScriptObject;
class ScriptState;

using ExtensionVector = blink::Vector<blink::String>;

// Handler for returning information around the available webgl extensions.
// This handler automatically takes into consideration any farbling when the
// brave shields are up.
class CORE_EXPORT WebGLExtensionHandler {
 public:
  WebGLExtensionHandler(const ExtensionVector& supported_extensions,
                        std::optional<std::uint8_t> fake_index = std::nullopt);
  ~WebGLExtensionHandler();

  // Returns a vector of supported extensions.
  ExtensionVector GetSupportedExtensions() const;

  // Return either |real_extension| or a farbled output.
  ScriptObject GetExtension(ScriptState* script_state,
                            const String& name,
                            const ScriptObject* real_extension);

 private:
  // Returns true if |extension_name| is a farbled value, false otherwise.
  bool IsExtensionFarbled(const String& extension_name) const;

  // The actual list of the supported webgl extensions.
  const ExtensionVector supported_extensions_;
  // Points to an index in the fake supported extensions array.
  std::optional<std::uint8_t> fake_index_;
};

// Returns the default handler without any farbling logic.
// |real_extensions| is the set of the actual supported WebGL extensions by the
// machine.
CORE_EXPORT std::unique_ptr<WebGLExtensionHandler> CreateOffHandler(
    const ExtensionVector& real_extensions);

// Return a handler which takes care of farbling.
// |real_extensions| is the set of the actual supported WebGL extensions by the
// machine.
// |create_maximum_handler| when set to true would create a handler with maximum
// fingerprinting protections, otherwise with default protections.
// |seed| this is derived from the current brave::FarblingToken for the session.
CORE_EXPORT std::unique_ptr<WebGLExtensionHandler> CreateFarblingHandler(
    const ExtensionVector& real_extensions,
    const bool create_maximum_handler,
    const uint64_t seed);
}  // namespace blink

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_BINDINGS_CORE_WEBGL_WEBGL_EXTENSION_HANDLER_H_
