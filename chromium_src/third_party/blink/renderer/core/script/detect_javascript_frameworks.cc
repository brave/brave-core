/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/script/detect_javascript_frameworks.h"

#define DetectJavascriptFrameworksOnLoad DetectJavascriptFrameworksOnLoad_Unused
#include "src/third_party/blink/renderer/core/script/detect_javascript_frameworks.cc"
#undef DetectJavascriptFrameworksOnLoad

namespace blink {

// NOLINTNEXTLINE(runtime/references)
void DetectJavascriptFrameworksOnLoad(Document& document) {}

}  // namespace blink
