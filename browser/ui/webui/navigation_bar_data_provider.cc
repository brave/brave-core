// Copyright (c) 2019 The Brave Authors
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/navigation_bar_data_provider.h"

#include "chrome/grit/generated_resources.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/web_ui_data_source.h"

#include "components/grit/brave_components_strings.h"

// static
void NavigationBarDataProvider::Initialize(content::WebUIDataSource* source) {
  source->AddLocalizedString(
    "brToolbarSettingsTitle", IDS_SETTINGS_SETTINGS);
  source->AddLocalizedString(
    "brToolbarBookmarksTitle", IDS_BOOKMARK_MANAGER_TITLE);
  source->AddLocalizedString(
    "brToolbarDownloadsTitle", IDS_DOWNLOAD_TITLE);
  source->AddLocalizedString(
    "brToolbarHistoryTitle", IDS_HISTORY_TITLE);
  source->AddLocalizedString(
    "brToolbarRewardsTitle", IDS_BRAVE_UI_BRAVE_REWARDS);
  source->AddLocalizedString(
    "brToolbarWalletsTitle", IDS_WALLETS_TITLE);
}

