// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/views/side_panel/customize_chrome/side_panel_controller_views.h"

#include "chrome/browser/ui/views/side_panel/side_panel_web_ui_view.h"
#include "chrome/browser/ui/webui/side_panel/customize_chrome/customize_chrome_ui.h"

// Inside SidePanelControllerViews::CreateCustomizeChromeWebView()
#define ShowUI()                                                           \
  ShowUI();                                                                \
  auto customize_chrome_ui = customize_chrome_web_view->contents_wrapper() \
                                 ->GetWebUIController()                    \
                                 ->GetWeakPtr();                           \
  CHECK(customize_chrome_ui);                                              \
  customize_chrome_ui->SetClosePanelCallback(                              \
      base::BindRepeating(&SidePanelControllerViews::CloseSidePanel,       \
                          weak_ptr_factory_.GetWeakPtr()));

#include "src/chrome/browser/ui/views/side_panel/customize_chrome/side_panel_controller_views.cc"

#undef ShowUI
