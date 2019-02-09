// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_BRAVE_PAGES_H_
#define BRAVE_BROWSER_UI_BRAVE_PAGES_H_

#include <string>

#include "url/gurl.h"

class Browser;

namespace brave {

void ShowBraveAdblock(Browser* browser);
void ShowBraveRewards(Browser* browser);
void ShowBraveSync(Browser* browser);
void ShowClearRewardsDataDialog(Browser* browser);
void ShowSettingsSubPage(Browser* browser, const std::string& sub_page);
void ShowSettingsSubPageInTabbedBrowser(Browser* browser,
                                        const std::string& sub_page);
GURL GetSettingsUrl(const std::string& sub_page);

}  // namespace brave

#endif  // BRAVE_BROWSER_UI_BRAVE_PAGES_H_
