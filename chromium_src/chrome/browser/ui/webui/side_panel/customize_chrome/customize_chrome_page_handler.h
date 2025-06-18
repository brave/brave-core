// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SIDE_PANEL_CUSTOMIZE_CHROME_CUSTOMIZE_CHROME_PAGE_HANDLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SIDE_PANEL_CUSTOMIZE_CHROME_CUSTOMIZE_CHROME_PAGE_HANDLER_H_

#include "chrome/browser/ui/webui/side_panel/customize_chrome/customize_chrome.mojom.h"

class CustomizeChromeUI;

#define UpdateNtpManagedByName                                       \
  UpdateNtpManagedByName() override;                                 \
  void set_customize_chrome_ui(                                      \
      const base::WeakPtr<CustomizeChromeUI>& customize_chrome_ui) { \
    customize_chrome_ui_ = std::move(customize_chrome_ui);           \
  }                                                                  \
                                                                     \
 private:                                                            \
  base::WeakPtr<CustomizeChromeUI> customize_chrome_ui_;             \
                                                                     \
 public:                                                             \
  void ClosePanel

#include "src/chrome/browser/ui/webui/side_panel/customize_chrome/customize_chrome_page_handler.h"  // IWYU pragma: export

#undef UpdateNtpManagedByName

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SIDE_PANEL_CUSTOMIZE_CHROME_CUSTOMIZE_CHROME_PAGE_HANDLER_H_
