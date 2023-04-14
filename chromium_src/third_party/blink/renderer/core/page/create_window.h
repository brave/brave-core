/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_PAGE_CREATE_WINDOW_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_PAGE_CREATE_WINDOW_H_

#include "third_party/blink/public/web/web_window_features.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

#define GetWindowFeaturesFromString                                      \
  GetWindowFeaturesFromString_ChromiumImpl(const String& feature_string, \
                                           LocalDOMWindow* dom_window);  \
  CORE_EXPORT WebWindowFeatures GetWindowFeaturesFromString

#include "src/third_party/blink/renderer/core/page/create_window.h"  // IWYU pragma: export

#undef GetWindowFeaturesFromString

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_PAGE_CREATE_WINDOW_H_
