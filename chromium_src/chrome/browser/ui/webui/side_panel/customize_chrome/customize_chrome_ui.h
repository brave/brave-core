// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SIDE_PANEL_CUSTOMIZE_CHROME_CUSTOMIZE_CHROME_UI_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SIDE_PANEL_CUSTOMIZE_CHROME_CUSTOMIZE_CHROME_UI_H_

#include "base/functional/callback_forward.h"
#include "chrome/browser/ui/webui/side_panel/customize_chrome/customize_chrome.mojom.h"

#define AttachedTabStateUpdated                                            \
  AttachedTabStateUpdated_Unused();                                        \
  void SetClosePanelCallback(base::RepeatingClosure close_panel_callback); \
  base::RepeatingClosure close_panel_callback() const {                    \
    return close_panel_callback_;                                          \
  }                                                                        \
                                                                           \
 private:                                                                  \
  base::RepeatingClosure close_panel_callback_;                            \
                                                                           \
 public:                                                                   \
  void AttachedTabStateUpdated

#define CreatePageHandler(...)            \
  CreatePageHandlerChromium(__VA_ARGS__); \
  void CreatePageHandler(__VA_ARGS__)

#include <chrome/browser/ui/webui/side_panel/customize_chrome/customize_chrome_ui.h>  // IWYU pragma: export

#undef CreatePageHandler
#undef AttachedTabStateUpdated

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SIDE_PANEL_CUSTOMIZE_CHROME_CUSTOMIZE_CHROME_UI_H_
