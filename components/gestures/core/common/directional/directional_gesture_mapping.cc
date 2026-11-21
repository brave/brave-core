// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/directional/directional_gesture_mapping.h"

#include <utility>

#include "base/no_destructor.h"
#include "brave/components/gestures/core/common/directional/directional_gesture_defaults.h"
#include "brave/components/gestures/core/common/directional/directional_gesture_key_util.h"

namespace gestures {

DirectionalGestureMapping::DirectionalGestureMapping(
    DirectionalGestureDefaultCommandsInfo default_commands)
    : default_commands_(default_commands),
      mappings_(DefaultMappings(default_commands_)) {}

DirectionalGestureMapping::~DirectionalGestureMapping() = default;

const GestureActions& DirectionalGestureMapping::ActionsForStrokes(
    const DirectionalGestureStrokes& strokes) const {
  static const base::NoDestructor<GestureActions> kNoActions;
  if (strokes.empty()) {
    return *kNoActions;
  }
  const auto iter = mappings_.find(ToDirectionalGestureStrokesKey(strokes));
  if (iter == mappings_.cend()) {
    return *kNoActions;
  }
  const auto& [_, actions] = *iter;
  return actions;
}

void DirectionalGestureMapping::SetActions(std::string gesture_key,
                                           GestureActions actions) {
  mappings_[std::move(gesture_key)] = std::move(actions);
}

void DirectionalGestureMapping::Remove(std::string_view gesture_key) {
  mappings_.erase(gesture_key);
}

void DirectionalGestureMapping::ResetToDefaults() {
  mappings_ = DefaultMappings(default_commands_);
}

}  // namespace gestures
