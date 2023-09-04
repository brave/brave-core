/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_DEVICE_CHOOSER_CONTENT_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_DEVICE_CHOOSER_CONTENT_VIEW_H_

class Browser;

#define CreateExtraView(...)    \
  CreateExtraView(__VA_ARGS__); \
  std::unique_ptr<views::View> CreateFootnoteView(Browser* browser)

#include "src/chrome/browser/ui/views/device_chooser_content_view.h"  // IWYU pragma: export

#undef CreateExtraView

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_DEVICE_CHOOSER_CONTENT_VIEW_H_
