// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/webui/side_panel/customize_chrome/customize_chrome_ui.h"

#include "base/feature_list.h"
#include "base/values.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/grit/generated_resources.h"
#include "components/grit/brave_components_webui_strings.h"
#include "components/search/ntp_features.h"
#include "content/public/browser/web_ui_data_source.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/webui/webui_util.h"

#if defined(TOOLKIT_VIEWS)
#include "brave/browser/ui/darker_theme/features.h"
#endif  // defined(TOOLKIT_VIEWS)

namespace {

// Adds Brave-specific values to the data source created by CustomizeChromeUI's
// constructor. WebUIDataSource::Update() takes already-resolved values, so
// localized strings are resolved here.
void UpdateBraveDataSource(Profile* profile) {
  base::DictValue update;
  update.Set("braveCustomizeMenuToolbarLabel",
             l10n_util::GetStringUTF16(IDS_BRAVE_CUSTOMIZE_MENU_TOOLBAR_LABEL));
  for (const auto& [name, id] : webui::kCustomizeChromeStrings) {
    update.Set(name, l10n_util::GetStringUTF16(id));
  }

#if defined(TOOLKIT_VIEWS)
  update.Set(
      "shouldShowDarkerThemeToggle",
      base::FeatureList::IsEnabled(darker_theme::features::kBraveDarkerTheme));
#endif  // defined(TOOLKIT_VIEWS)

  content::WebUIDataSource::Update(
      profile, chrome::kChromeUICustomizeChromeSidePanelHost,
      std::move(update));
}

}  // namespace

#include <chrome/browser/ui/webui/side_panel/customize_chrome/customize_chrome_ui.cc>

void CustomizeChromeUI::SetClosePanelCallback(
    base::RepeatingClosure close_panel_callback) {
  close_panel_callback_ = std::move(close_panel_callback);
}
