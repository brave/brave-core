// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/webui/app_settings/web_app_settings_ui.h"

#include "base/feature_list.h"
#include "chrome/common/chrome_features.h"
#include "content/public/browser/web_ui_data_source.h"
#include "ui/webui/webui_util.h"

#define SetupWebUIDataSource(...)      \
  SetupWebUIDataSource(__VA_ARGS__);   \
  html_source->AddBoolean(             \
      "isPWAsTabStripSettingsEnabled", \
      base::FeatureList::IsEnabled(features::kDesktopPWAsTabStripSettings))

#define AddLocalizedStrings(...)                                        \
  AddLocalizedStrings(__VA_ARGS__);                                     \
  html_source->AddLocalizedString("appManagementTabbedWindowModeLabel", \
                                  IDS_APP_MANAGEMENT_TABBED_WINDOW);    \
  html_source->AddLocalizedString("appManagementBrowserModeLabel",      \
                                  IDS_APP_MANAGEMENT_BROWSER);          \
  html_source->AddLocalizedString("appManagementOpenModeLabel",         \
                                  IDS_APP_MANAGEMENT_OPEN_MODE)

#include <chrome/browser/ui/webui/app_settings/web_app_settings_ui.cc>

#undef AddLocalizedStrings
#undef SetupWebUIDataSource
