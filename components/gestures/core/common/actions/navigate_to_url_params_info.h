// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_NAVIGATE_TO_URL_PARAMS_INFO_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_NAVIGATE_TO_URL_PARAMS_INFO_H_

#include <string>

namespace gestures {

struct NavigateToUrlParamsInfo {
  bool operator==(const NavigateToUrlParamsInfo&) const = default;

  std::string url_spec;
};

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_NAVIGATE_TO_URL_PARAMS_INFO_H_
