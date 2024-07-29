/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_rewards/rewards_web_ui_utils.h"

#include "brave/browser/brave_rewards/rewards_util.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/constants/webui_url_constants.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "url/gurl.h"

namespace brave_rewards {

bool ShouldBlockRewardsWebUI(content::BrowserContext* browser_context,
                             const GURL& url) {
  if (url.host_piece() != kRewardsPageHost &&
#if !BUILDFLAG(IS_ANDROID)
      url.host_piece() != kRewardsPageTopHost &&
      url.host_piece() != kBraveRewardsPanelHost &&
      url.host_piece() != kBraveTipPanelHost &&
#endif  // !BUILDFLAG(IS_ANDROID)
      url.host_piece() != kRewardsInternalsHost) {
    return false;
  }

  Profile* profile = Profile::FromBrowserContext(browser_context);
  if (profile) {
    if (!brave_rewards::IsSupportedForProfile(
            profile, url.host_piece() == kRewardsPageHost
                         ? brave_rewards::IsSupportedOptions::kSkipRegionCheck
                         : brave_rewards::IsSupportedOptions::kNone)) {
      return true;
    }
#if BUILDFLAG(IS_ANDROID)
    auto* prefs = profile->GetPrefs();
    if (prefs && prefs->GetBoolean(kSafetynetCheckFailed)) {
      return true;
    }
#endif  // BUILDFLAG(IS_ANDROID)
  }
  return false;
}

}  // namespace brave_rewards
