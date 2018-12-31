/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_AUTO_CONTRIBUTION_PROPS_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_AUTO_CONTRIBUTION_PROPS_H_

#include <stdint.h>

namespace brave_rewards {

struct AutoContributeProps {
  AutoContributeProps();
  ~AutoContributeProps();
  AutoContributeProps(const AutoContributeProps& properties) = default;

  bool enabled_contribute;
  uint64_t contribution_min_time;
  int32_t contribution_min_visits;
  bool contribution_non_verified;
  bool contribution_videos;
  uint64_t reconcile_stamp;
};

}  // namespace brave_rewards

#endif //BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_AUTO_CONTRIBUTION_PROPS_H_
