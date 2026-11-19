// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/webui/side_panel/customize_chrome/customize_chrome_ui.h"

#include "chrome/grit/generated_resources.h"
#include "components/grit/brave_components_webui_strings.h"
#include "components/search/ntp_features.h"
#include "content/public/browser/web_ui_data_source.h"
#include "ui/webui/webui_util.h"

#if defined(TOOLKIT_VIEWS)
#include "brave/browser/ui/darker_theme/features.h"
#endif  // defined(TOOLKIT_VIEWS)

#include <chrome/browser/ui/webui/side_panel/customize_chrome/customize_chrome_ui.cc>

void CustomizeChromeUI::SetClosePanelCallback(
    base::RepeatingClosure close_panel_callback) {
  close_panel_callback_ = std::move(close_panel_callback);
}
