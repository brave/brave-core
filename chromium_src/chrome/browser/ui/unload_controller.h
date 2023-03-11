/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_UNLOAD_CONTROLLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_UNLOAD_CONTROLLER_H_

#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"

#define TabStripEmpty           \
  TabStripEmpty_ChromiumImpl(); \
  void TabStripEmpty
#include "src/chrome/browser/ui/unload_controller.h"  // IWYU pragma: export
#undef TabStripEmpty

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_UNLOAD_CONTROLLER_H_
