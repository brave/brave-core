/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_RENDERER_HOST_CHROME_NAVIGATION_UI_DATA_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_RENDERER_HOST_CHROME_NAVIGATION_UI_DATA_H_

#define url_is_typed_with_http_scheme() \
  return_false() const {                \
    return false;                       \
  }                                     \
  bool url_is_typed_with_http_scheme()

#include "src/chrome/browser/renderer_host/chrome_navigation_ui_data.h"  // IWYU pragma: export

#undef url_is_typed_with_http_scheme

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_RENDERER_HOST_CHROME_NAVIGATION_UI_DATA_H_
