// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_MAPPING_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_MAPPING_H_

#include <functional>
#include <string>
#include <string_view>

#include "base/containers/flat_map.h"
#include "brave/components/gestures/core/common/actions/gesture_action_info.h"
#include "brave/components/gestures/core/common/directional/directional_gesture_default_commands_info.h"
#include "brave/components/gestures/core/common/directional/directional_gesture_types.h"

namespace gestures {

using DirectionalGestureMappings =
    base::flat_map<std::string, GestureActions, std::less<>>;

// Holds the gesture-to-actions mapping table, keyed by each gesture's
// `ToDirectionalGestureStrokesKey` string form, such as `←` or `↓→`.
class DirectionalGestureMapping final {
 public:
  // `default_commands` seeds the initial table and is reused by
  // `ResetToDefaults()`, since this component can't depend on `IDC_*` headers.
  explicit DirectionalGestureMapping(
      DirectionalGestureDefaultCommandsInfo default_commands);

  DirectionalGestureMapping(const DirectionalGestureMapping&) = delete;
  DirectionalGestureMapping& operator=(const DirectionalGestureMapping&) =
      delete;

  ~DirectionalGestureMapping();

  // Returns the actions mapped to `strokes`, in run order, if any mapping
  // (default or user-set) matches. Empty if nothing matches.
  const GestureActions& ActionsForStrokes(
      const DirectionalGestureStrokes& strokes) const;

  // Adds or overwrites the mapping for `gesture_key`, e.g. for a user remapping
  // a gesture from the settings UI. `actions` must not be empty.
  void SetActions(std::string gesture_key, GestureActions actions);

  // Removes any mapping for `gesture_key`, default or user-set, so the gesture
  // no longer matches to anything.
  void Remove(std::string_view gesture_key);

  // Discards all user overrides and restores the built-in mapping table.
  void ResetToDefaults();

 private:
  const DirectionalGestureDefaultCommandsInfo default_commands_;

  // `std::less<>` makes lookup/erase transparent, so queries can use a
  // `std::string_view` without constructing a `std::string` first.
  DirectionalGestureMappings mappings_;
};

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_DIRECTIONAL_DIRECTIONAL_GESTURE_MAPPING_H_
