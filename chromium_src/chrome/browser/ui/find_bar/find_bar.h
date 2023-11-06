/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_FIND_BAR_FIND_BAR_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_FIND_BAR_FIND_BAR_H_

#include "build/build_config.h"

#if BUILDFLAG(IS_WIN)
namespace views {
class Widget;
}

#define UpdateFindBarForChangedWebContents  \
  UpdateFindBarForChangedWebContents() = 0; \
  virtual views::Widget* GetHostWidget
#endif

#include "src/chrome/browser/ui/find_bar/find_bar.h"  // IWYU pragma: export

#if BUILDFLAG(IS_WIN)
#undef UpdateFindBarForChangedWebContents
#endif

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_FIND_BAR_FIND_BAR_H_
