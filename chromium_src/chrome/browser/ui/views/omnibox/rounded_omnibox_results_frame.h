/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_OMNIBOX_ROUNDED_OMNIBOX_RESULTS_FRAME_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_OMNIBOX_ROUNDED_OMNIBOX_RESULTS_FRAME_H_

#define GetShadowInsets \
  GetShadowInsets();    \
  static int GetShadowElevation

#include "src/chrome/browser/ui/views/omnibox/rounded_omnibox_results_frame.h"  // IWYU pragma: export

#undef GetShadowInsets

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_OMNIBOX_ROUNDED_OMNIBOX_RESULTS_FRAME_H_
