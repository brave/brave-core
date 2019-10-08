/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_PROTOCOL_HANDLER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_PROTOCOL_HANDLER_H_

#include <string>

#include "chrome/browser/external_protocol/external_protocol_handler.h"
#include "chrome/browser/profiles/profile.h"

namespace brave_rewards {

bool HandleRewardsProtocol(
    const GURL& url,
    content::WebContents::Getter web_contents_getter,
    ui::PageTransition page_transition,
    bool has_user_gesture);

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_PROTOCOL_HANDLER_H_
