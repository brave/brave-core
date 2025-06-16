// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "src/chrome/browser/ui/webui/side_panel/customize_chrome/customize_chrome_page_handler.cc"

void CustomizeChromePageHandler::ClosePanel() {
  if (close_panel_callback_) {
    close_panel_callback_.Run();
  }
}
