/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_WEBGL_WEBGL_SHADER_PRECISION_FORMAT_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_WEBGL_WEBGL_SHADER_PRECISION_FORMAT_H_

#include "src/third_party/blink/renderer/modules/webgl/webgl_shader_precision_format.h"

#include "third_party/blink/renderer/modules/modules_export.h"

namespace blink {

MODULES_EXPORT String
ToPageGraphBlinkArg(const WebGLShaderPrecisionFormat* plugins);

}  // namespace blink

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_MODULES_WEBGL_WEBGL_SHADER_PRECISION_FORMAT_H_
