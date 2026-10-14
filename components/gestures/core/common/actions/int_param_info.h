// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_INT_PARAM_INFO_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_INT_PARAM_INFO_H_

namespace gestures {

// A single int-valued param shared by every command that takes one (e.g.
// Open Settings' target page). The caller casts `value` to whatever
// enum its own command actually stores, validating range itself; this
// component has no knowledge of what the int means.
struct IntParamInfo {
  bool operator==(const IntParamInfo&) const = default;

  int value = 0;
};

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_INT_PARAM_INFO_H_
