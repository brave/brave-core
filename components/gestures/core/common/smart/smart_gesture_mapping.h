// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_SMART_SMART_GESTURE_MAPPING_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_SMART_SMART_GESTURE_MAPPING_H_

#include <string>
#include <string_view>

#include "base/containers/flat_map.h"
#include "brave/components/gestures/core/common/actions/gesture_action_info.h"
#include "brave/components/gestures/core/common/smart/smart_gesture_content_type.h"
#include "brave/components/gestures/core/common/smart/smart_gesture_direction_util.h"

namespace gestures {

// `std::less<>` makes lookup and erase transparent, so callers can query
// with a `std::string_view` without constructing a `std::string` first.
using SmartGestureMappings =
    base::flat_map<std::string, GestureActions, std::less<>>;

// Holds the smart-gesture-to-actions mapping table, keyed by each
// content-type/direction pair's `SmartGestureKey` string form, such as
// `"link:right"`. Mirrors `DirectionalGestureMapping`'s shape, just with a
// much smaller, hardcoded default table, since there's no browser `IDC_*`
// command involved to inject.
class SmartGestureMapping final {
 public:
  SmartGestureMapping();

  SmartGestureMapping(const SmartGestureMapping&) = delete;
  SmartGestureMapping& operator=(const SmartGestureMapping&) = delete;

  ~SmartGestureMapping();

  // The actions mapped to `content_type`/`direction_type`, if any mapping
  // (default or user-set) matches. Empty if nothing matches.
  const GestureActions& ActionsFor(
      SmartGestureContentType content_type,
      SmartGestureDirectionType direction_type) const;

  // Adds or overwrites the mapping for `key` (see `SmartGestureKey`), e.g.
  // for a user remapping a slot from the settings UI. `actions` must not be
  // empty.
  void SetChainedActions(std::string key, GestureActions actions);

  // Removes any mapping for `key`, default or user-set, so that
  // content-type/direction pair no longer matches to anything.
  void Remove(std::string_view key);

  // Discards all user overrides and restores the built-in mapping table.
  void ResetToDefaults();

 private:
  SmartGestureMappings mappings_;
};

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_SMART_SMART_GESTURE_MAPPING_H_
