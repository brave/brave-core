/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_WEBGL_WEBGL_RENDERING_CONTEXT_BASE_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_WEBGL_WEBGL_RENDERING_CONTEXT_BASE_H_

#define getExtension                                           \
  getExtension_ChromiumImpl(ScriptState*, const String& name); \
  ScriptValue getExtension

#define getSupportedExtensions           \
  getSupportedExtensions_ChromiumImpl(); \
  absl::optional<Vector<String>> getSupportedExtensions

#include "../../../../../../../third_party/blink/renderer/modules/webgl/webgl_rendering_context_base.h"

#undef getSupportedExtensions
#undef getExtension

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_WEBGL_WEBGL_RENDERING_CONTEXT_BASE_H_
