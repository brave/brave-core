/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_ABOUT_ABOUT_UI_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_ABOUT_ABOUT_UI_H_

namespace content {
class BrowserContext;
}  // namespace content

#define FinishDataRequest                                                 \
  NotUsed() {}                                                            \
  std::string ChromeURLs(content::BrowserContext* browser_context) const; \
  void FinishDataRequest

#include "src/chrome/browser/ui/webui/about/about_ui.h"  // IWYU pragma: export

#undef FinishDataRequest

#endif  //  BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_ABOUT_ABOUT_UI_H_
