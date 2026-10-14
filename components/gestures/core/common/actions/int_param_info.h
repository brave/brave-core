// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_INT_PARAM_INFO_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_INT_PARAM_INFO_H_

namespace gestures {

// A single int-valued param shared by every command that takes one; the
// caller casts `value` to its own enum, validating range itself.
struct IntParamInfo final {
  bool operator==(const IntParamInfo&) const = default;

  int value = 0;
};

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_INT_PARAM_INFO_H_
