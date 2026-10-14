// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_TEXT_PARAM_INFO_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_TEXT_PARAM_INFO_H_

#include <string>

namespace gestures {

// A single free-text param shared by every command that takes one; which it
// is depends on `GestureActionInfo::command`.
struct TextParamInfo final {
  bool operator==(const TextParamInfo&) const = default;

  std::string value;
};

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_TEXT_PARAM_INFO_H_
