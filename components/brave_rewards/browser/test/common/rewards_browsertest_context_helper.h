/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_TEST_COMMON_REWARDS_BROWSERTEST_CONTEXT_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_TEST_COMMON_REWARDS_BROWSERTEST_CONTEXT_HELPER_H_

#include <vector>

#include "brave/components/brave_rewards/browser/test/common/rewards_browsertest_util.h"
#include "build/build_config.h"
#include "chrome/browser/ui/browser.h"
#include "url/gurl.h"

namespace content {
class WebContents;
}  // namespace content

namespace rewards_browsertest_helper {

content::WebContents* OpenRewardsPopup(Browser* browser);

void EnableRewards(Browser* browser, const bool use_new_tab = false);

content::WebContents* OpenSiteBanner(
    Browser* browser,
    rewards_browsertest_util::ContributionType banner_type);

void VisitPublisher(
    Browser* browser,
    const GURL& url,
    const bool verified,
    const bool last_add = false);

void LoadURL(Browser* browser, GURL url);

void ReloadCurrentSite(Browser* browser);

}  // namespace rewards_browsertest_helper

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_TEST_COMMON_REWARDS_BROWSERTEST_CONTEXT_HELPER_H_
