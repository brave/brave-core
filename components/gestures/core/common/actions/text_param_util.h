// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_TEXT_PARAM_UTIL_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_TEXT_PARAM_UTIL_H_

#include <string_view>

#include "brave/components/gestures/core/common/actions/text_param_info.h"

namespace base {
class DictValue;
}  // namespace base

namespace gestures {

// The wire key read by `ParseTextParam`, shared by every command that takes
// a text param. Kept in sync with `toActionDict` in `gesture_rows.ts`.
inline constexpr std::string_view kTextParamKey = "text_param";

// Reads `kTextParamKey` from `dict`, defaulting to an empty string if
// missing or not a string.
TextParamInfo ParseTextParam(const base::DictValue& dict);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_TEXT_PARAM_UTIL_H_
