/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/auto_contribution_props.h"

namespace brave_rewards {

AutoContributeProps::AutoContributeProps()
  : enabled_contribute(false),
    contribution_min_time(0),
    contribution_min_visits(0),
    contribution_non_verified(false),
    contribution_videos(false),
    reconcile_stamp(0) {
}

AutoContributeProps::~AutoContributeProps() { }

}  // namespace brave_rewards
