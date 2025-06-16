// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SIDE_PANEL_CUSTOMIZE_CHROME_CUSTOMIZE_CHROME_PAGE_HANDLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SIDE_PANEL_CUSTOMIZE_CHROME_CUSTOMIZE_CHROME_PAGE_HANDLER_H_

#include "chrome/browser/ui/webui/side_panel/customize_chrome/customize_chrome.mojom.h"

#define UpdateNtpManagedByName                                                 \
  UpdateNtpManagedByName() override;                                           \
  void set_close_panel_callback(base::RepeatingClosure close_panel_callback) { \
    close_panel_callback_ = std::move(close_panel_callback);                   \
  }                                                                            \
                                                                               \
 private:                                                                      \
  base::RepeatingClosure close_panel_callback_;                                \
                                                                               \
 public:                                                                       \
  void ClosePanel

#include "src/chrome/browser/ui/webui/side_panel/customize_chrome/customize_chrome_page_handler.h"  // IWYU pragma: export

#undef UpdateNtpManagedByName

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SIDE_PANEL_CUSTOMIZE_CHROME_CUSTOMIZE_CHROME_PAGE_HANDLER_H_
