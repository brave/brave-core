// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_SWITCH_TO_TAB_PARAMS_INFO_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_SWITCH_TO_TAB_PARAMS_INFO_H_

#include <string>

namespace gestures {

struct SwitchToTabParamsInfo {
  bool operator==(const SwitchToTabParamsInfo&) const = default;

  // A glob pattern (`*`/`?`, see `base::MatchPattern`) matched against each
  // open tab's URL or title to find one to switch to.
  std::string pattern;
};

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_SWITCH_TO_TAB_PARAMS_INFO_H_
