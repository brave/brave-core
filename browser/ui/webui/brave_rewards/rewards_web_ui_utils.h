/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_REWARDS_WEB_UI_UTILS_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_REWARDS_WEB_UI_UTILS_H_

namespace content {
class BrowserContext;
}

class GURL;

namespace brave_rewards {

bool ShouldBlockRewardsWebUI(content::BrowserContext* browser_context,
                             const GURL& url);

}  // namespace brave_rewards

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_REWARDS_REWARDS_WEB_UI_UTILS_H_
