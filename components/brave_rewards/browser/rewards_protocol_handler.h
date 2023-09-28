/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_PROTOCOL_HANDLER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_PROTOCOL_HANDLER_H_

#include "content/public/browser/web_contents.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

namespace brave_rewards {

bool IsRewardsProtocol(const GURL& url);

void HandleRewardsProtocol(const GURL& url,
                           content::WebContents* web_contents,
                           ui::PageTransition page_transition);

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_PROTOCOL_HANDLER_H_
