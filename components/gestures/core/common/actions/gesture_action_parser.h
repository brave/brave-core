// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_GESTURE_ACTION_PARSER_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_GESTURE_ACTION_PARSER_H_

#include <optional>

#include "base/values.h"
#include "brave/components/gestures/core/common/actions/gesture_action_info.h"

namespace gestures {

// Parses an actions list (e.g. `kDirectionalGestures`'s value); invalid
// entries are skipped, not discarding the whole list. `nullopt` if empty.
std::optional<GestureActions> MaybeParseGestureActions(
    const base::ListValue& list);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_GESTURE_ACTION_PARSER_H_
