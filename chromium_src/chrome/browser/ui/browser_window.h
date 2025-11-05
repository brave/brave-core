/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BROWSER_WINDOW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BROWSER_WINDOW_H_

#include "build/build_config.h"

// Guard browser_window.h include since it includes tab_search.mojom.h
// which is desktop-only
#if !BUILDFLAG(IS_ANDROID)
#include <chrome/browser/ui/browser_window.h>  // IWYU pragma: export
#endif  // !BUILDFLAG(IS_ANDROID)

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_BROWSER_WINDOW_H_

