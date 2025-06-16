// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/webui/side_panel/customize_chrome/customize_chrome_ui.h"

#include "content/public/browser/web_ui_data_source.h"

#define AddLocalizedStrings(...)                               \
  AddLocalizedStrings(__VA_ARGS__);                            \
  source->AddLocalizedString("braveCustomizeMenuToolbarLabel", \
                             IDS_BRAVE_CUSTOMIZE_MENU_TOOLBAR_LABEL)
#define CreatePageHandler CreatePageHandlerChromium

#include "src/chrome/browser/ui/webui/side_panel/customize_chrome/customize_chrome_ui.cc"

#undef CreatePageHandler
#undef AddLocalizedStrings

void CustomizeChromeUI::CreatePageHandler(
    mojo::PendingRemote<side_panel::mojom::CustomizeChromePage> pending_page,
    mojo::PendingReceiver<side_panel::mojom::CustomizeChromePageHandler>
        pending_page_handler) {
  CreatePageHandlerChromium(std::move(pending_page),
                            std::move(pending_page_handler));
  CHECK(customize_chrome_page_handler_);
  customize_chrome_page_handler_->set_close_panel_callback(
      close_panel_callback_);
}

void CustomizeChromeUI::SetClosePanelCallback(
    base::RepeatingClosure close_panel_callback) {
  close_panel_callback_ = std::move(close_panel_callback);
  if (customize_chrome_page_handler_) {
    // Do not std::move() callback as handler may be created after this call
    customize_chrome_page_handler_->set_close_panel_callback(
        close_panel_callback);
  }
}
