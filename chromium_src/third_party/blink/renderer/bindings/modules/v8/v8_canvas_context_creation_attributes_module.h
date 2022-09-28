/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_BINDINGS_MODULES_V8_V8_CANVAS_CONTEXT_CREATION_ATTRIBUTES_MODULE_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_BINDINGS_MODULES_V8_V8_CANVAS_CONTEXT_CREATION_ATTRIBUTES_MODULE_H_

#include "../gen/third_party/blink/renderer/bindings/modules/v8/v8_canvas_context_creation_attributes_module.h"

#include "third_party/blink/renderer/modules/modules_export.h"

namespace blink {

MODULES_EXPORT String
ToPageGraphBlinkArg(CanvasContextCreationAttributesModule* attributes);

}  // namespace blink

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_BINDINGS_MODULES_V8_V8_CANVAS_CONTEXT_CREATION_ATTRIBUTES_MODULE_H_
