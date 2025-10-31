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

// Add Brave-specific strings to the WebUI data source.
#define AddLocalizedStrings(...)                                      \
  AddLocalizedStrings(__VA_ARGS__);                                   \
  source->AddLocalizedString("braveCustomizeMenuToolbarLabel",        \
                             IDS_BRAVE_CUSTOMIZE_MENU_TOOLBAR_LABEL); \
  source->AddLocalizedStrings(webui::kCustomizeChromeStrings)
#define SetupWebUIDataSource(...)    \
  SetupWebUIDataSource(__VA_ARGS__); \
  source->AddBoolean("showDeviceThemeToggle", false)
#define CreatePageHandler CreatePageHandlerChromium

// Replaces IDS_NTP_CUSTOMIZE_APPEARANCE_LABEL with a Brave-specific label.
#undef IDS_NTP_CUSTOMIZE_APPEARANCE_LABEL
#define IDS_NTP_CUSTOMIZE_APPEARANCE_LABEL \
  IDS_BRAVE_NTP_CUSTOMIZE_APPEARANCE_LABEL

#if defined(TOOLKIT_VIEWS)
// Add a boolean data for DarkerTheme mode to WebUI data source.
#define kNtpFooter kNtpFooter));     \
  source->AddBoolean(                \
      "shouldShowDarkerThemeToggle", \
      base::FeatureList::IsEnabled(darker_theme::features::kBraveDarkerTheme
#endif  // defined(TOOLKIT_VIEWS)

#include <chrome/browser/ui/webui/side_panel/customize_chrome/customize_chrome_ui.cc>

#if defined(TOOLKIT_VIEWS)
#undef kNtpFooter
#endif  // defined(TOOLKIT_VIEWS)
#undef IDS_NTP_CUSTOMIZE_APPEARANCE_LABEL
#undef CreatePageHandler
#undef SetupWebUIDataSource
#undef AddLocalizedStrings

void CustomizeChromeUI::CreatePageHandler(
    mojo::PendingRemote<side_panel::mojom::CustomizeChromePage> pending_page,
    mojo::PendingReceiver<side_panel::mojom::CustomizeChromePageHandler>
        pending_page_handler) {
  CreatePageHandlerChromium(std::move(pending_page),
                            std::move(pending_page_handler));
  CHECK(customize_chrome_page_handler_);
  customize_chrome_page_handler_->set_customize_chrome_ui(
      weak_ptr_factory_.GetWeakPtr());
}

void CustomizeChromeUI::SetClosePanelCallback(
    base::RepeatingClosure close_panel_callback) {
  close_panel_callback_ = std::move(close_panel_callback);
}
