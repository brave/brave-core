/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FIND_BAR_HOST_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FIND_BAR_HOST_H_

#include "build/build_config.h"
#include "chrome/browser/ui/find_bar/find_bar.h"

#if BUILDFLAG(IS_WIN)
#define UpdateFindBarForChangedWebContents       \
  UpdateFindBarForChangedWebContents() override; \
  views::Widget* GetHostWidget
#endif

#include "src/chrome/browser/ui/views/find_bar_host.h"  // IWYU pragma: export

#if BUILDFLAG(IS_WIN)
#undef UpdateFindBarForChangedWebContents
#endif

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_FIND_BAR_HOST_H_
