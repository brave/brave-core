// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_DEFAULTS_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_DEFAULTS_H_

#include <functional>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "brave/components/gestures/core/common/actions/gesture_action_info.h"

namespace gestures {

// The built-in gesture-to-actions bindings, keyed by
// `DirectionToGestureKeyCode`-coded stroke sequences. Chosen so the most
// commonly
// used actions are assigned to the simplest strokes.
base::flat_map<std::string, std::vector<GestureActionInfo>, std::less<>>
DefaultMappings();

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_DEFAULTS_H_
