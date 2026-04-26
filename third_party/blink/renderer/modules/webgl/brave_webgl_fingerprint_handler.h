/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_MODULES_WEBGL_BRAVE_WEBGL_FINGERPRINT_HANDLER_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_MODULES_WEBGL_BRAVE_WEBGL_FINGERPRINT_HANDLER_H_

#include <memory>

#include "brave/third_party/blink/renderer/brave_farbling_constants.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

using ExtensionVector = blink::Vector<blink::String>;

// Abstract base class for BraveWebGLFingerprintHandler.
class BraveWebGLFingerprintHandler {
 public:
  virtual ~BraveWebGLFingerprintHandler() = default;
  virtual ExtensionVector GetSupportedExtensions() const = 0;
  virtual bool IsExtensionSupported(const String& extension_name) const = 0;
};

std::unique_ptr<BraveWebGLFingerprintHandler> CreateWebGLFingerprintHandler(
    const ExtensionVector& real_extensions,
    BraveFarblingLevel farbling_level);

}  // namespace blink
#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_MODULES_WEBGL_BRAVE_WEBGL_FINGERPRINT_HANDLER_H_
