/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_MODULES_WEBGL_BRAVE_WEBGL_FINGERPRINT_HANDLER_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_MODULES_WEBGL_BRAVE_WEBGL_FINGERPRINT_HANDLER_H_

#include <memory>
#include <optional>

#include "brave/components/brave_shields/content/browser/brave_farbling_service.h"
#include "brave/third_party/blink/renderer/brave_farbling_constants.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

class String;
class ScriptObject;
class ScriptState;

using ExtensionVector = blink::Vector<blink::String>;

// TODO: Add documentation.
class CORE_EXPORT BraveWebGLFingerprintHandler {
 public:
  BraveWebGLFingerprintHandler(
      const ExtensionVector& supported_extensions,
      std::optional<std::uint8_t> fake_index = std::nullopt);
  ~BraveWebGLFingerprintHandler();

  ExtensionVector GetSupportedExtensions() const;
  ScriptObject GetExtension(ScriptState* script_state, const String& name);

  bool IsFarbledExtension(const String& extension_name) const;
  bool IsExtensionSupported(const String& extension_name) const;

 private:
  const ExtensionVector supported_extensions_;
  std::optional<std::uint8_t> fake_index_;
};

// TODO: Add documentation.
CORE_EXPORT std::unique_ptr<BraveWebGLFingerprintHandler>
CreateWebGLFingerprintHandler(const ExtensionVector& real_extensions,
                              BraveFarblingLevel farbling_level);

}  // namespace blink

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_MODULES_WEBGL_BRAVE_WEBGL_FINGERPRINT_HANDLER_H_
