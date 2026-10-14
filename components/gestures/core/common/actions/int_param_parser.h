// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_INT_PARAM_PARSER_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_INT_PARAM_PARSER_H_

#include <string_view>

#include "brave/components/gestures/core/common/actions/int_param_info.h"

namespace base {
class DictValue;
}  // namespace base

namespace gestures {

// The wire key read by `ParseIntParam`, shared by every command that takes
// an int param.
inline constexpr std::string_view kValueParamKey = "value_param";

// Reads `kValueParamKey` from `dict`, defaulting to 0 if missing or not an int.
IntParamInfo ParseIntParam(const base::DictValue& dict);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_INT_PARAM_PARSER_H_
