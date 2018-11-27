/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_NET_NETWORK_HELPER_DELEGATE_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_NET_NETWORK_HELPER_DELEGATE_H_

#include "brave/browser/net/url_context.h"

namespace brave_rewards {

int OnBeforeURLRequest(
    const brave::ResponseCallback& next_callback,
    std::shared_ptr<brave::BraveRequestInfo> ctx);

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_NET_NETWORK_HELPER_DELEGATE_H_
