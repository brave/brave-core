// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/navigation_bar_data_provider.h"

#include "brave/browser/brave_rewards/rewards_util.h"
#include "brave/browser/brave_wallet/brave_wallet_context_utils.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/grit/generated_resources.h"
#include "components/grit/brave_components_strings.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/web_ui_data_source.h"

// static
void NavigationBarDataProvider::Initialize(content::WebUIDataSource* source,
                                           Profile* profile) {
  source->AddLocalizedString("brToolbarSettingsTitle", IDS_SETTINGS_SETTINGS);
  source->AddLocalizedString("brToolbarBookmarksTitle",
                             IDS_BOOKMARK_MANAGER_TITLE);
  source->AddLocalizedString("brToolbarDownloadsTitle", IDS_DOWNLOADS_TITLE);
  source->AddLocalizedString("brToolbarHistoryTitle", IDS_HISTORY_TITLE);
  source->AddLocalizedString("brToolbarRewardsTitle",
                             IDS_BRAVE_UI_BRAVE_REWARDS);
  source->AddLocalizedString("brToolbarWalletsTitle", IDS_WALLETS_TITLE);

  source->AddBoolean(
      "brToolbarShowRewardsButton",
      brave_rewards::IsSupportedForProfile(
          profile, brave_rewards::IsSupportedOptions::kSkipRegionCheck));
  source->AddBoolean("isBraveWalletAllowed",
                     brave_wallet::IsAllowedForContext(profile));
}
