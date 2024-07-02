/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_REWARDS_PAGE_DATA_SOURCE_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_REWARDS_PAGE_DATA_SOURCE_H_

#include <string>

namespace content {
class WebUI;
}

namespace brave_rewards {

// Creates a WebUIDataSource for the Rewards page and adds it to the browser
// context.
void CreateAndAddRewardsPageDataSource(content::WebUI& web_ui,
                                       const std::string& host);

}  // namespace brave_rewards

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_REWARDS_PAGE_DATA_SOURCE_H_
