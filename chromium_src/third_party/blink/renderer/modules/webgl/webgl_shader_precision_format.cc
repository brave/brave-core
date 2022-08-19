/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/third_party/blink/renderer/modules/webgl/webgl_shader_precision_format.cc"

#include <sstream>

#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

String ToPageGraphBlinkArg(const WebGLShaderPrecisionFormat* result) {
  std::stringstream buffer;
  buffer << "rangeMin: " << result->rangeMin()
         << ", rangeMax: " << result->rangeMax()
         << ", precision: " << result->precision();

  return String(buffer.str());
}

}  // namespace blink
