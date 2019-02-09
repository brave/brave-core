// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/brave_pages.h"

#include <string>

#include "brave/common/webui_url_constants.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/singleton_tabs.h"
#include "url/gurl.h"

namespace brave {

void ShowBraveRewards(Browser* browser) {
  ShowSingletonTabOverwritingNTP(
      browser,
      GetSingletonTabNavigateParams(browser, GURL(kBraveUIRewardsURL)));
}

void ShowBraveAdblock(Browser* browser) {
  ShowSingletonTabOverwritingNTP(
      browser,
      GetSingletonTabNavigateParams(browser, GURL(kBraveUIAdblockURL)));
}

void ShowBraveSync(Browser* browser) {
   ShowSingletonTabOverwritingNTP(
      browser,
      GetSingletonTabNavigateParams(browser, GURL(kBraveUISyncURL)));
}

void ShowClearRewardsDataDialog(Browser* browser) {
  ShowSettingsSubPage(browser, kClearRewardsDataSubPage);
}

void ShowSettingsSubPage(Browser* browser, const std::string& sub_page) {
  ShowSettingsSubPageInTabbedBrowser(browser, sub_page);
}

void ShowSettingsSubPageInTabbedBrowser(Browser* browser,
                                        const std::string& sub_page) {
  GURL gurl = GetSettingsUrl(sub_page);
  NavigateParams params(GetSingletonTabNavigateParams(browser, gurl));
  params.path_behavior = NavigateParams::IGNORE_AND_NAVIGATE;
  ShowSingletonTabOverwritingNTP(browser, std::move(params));
}

GURL GetSettingsUrl(const std::string& sub_page) {
  return GURL(std::string("brave://settings/") + sub_page);
}

}  // namespace brave
