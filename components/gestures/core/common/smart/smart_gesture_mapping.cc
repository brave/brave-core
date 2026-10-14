// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/smart/smart_gesture_mapping.h"

#include <utility>

#include "base/no_destructor.h"
#include "brave/components/gestures/core/common/smart/smart_gesture_defaults.h"
#include "brave/components/gestures/core/common/smart/smart_gesture_key_util.h"

namespace gestures {

SmartGestureMapping::SmartGestureMapping()
    : mappings_(DefaultSmartGestureMappings()) {}

SmartGestureMapping::~SmartGestureMapping() = default;

const GestureActions& SmartGestureMapping::ActionsFor(
    SmartGestureContentType content_type,
    SmartGestureDirectionType direction_type) const {
  static const base::NoDestructor<GestureActions> kNoActions;
  const auto iter =
      mappings_.find(SmartGestureKey(content_type, direction_type));
  if (iter == mappings_.end()) {
    return *kNoActions;
  }
  const auto& [_, actions] = *iter;
  return actions;
}

void SmartGestureMapping::SetChainedActions(std::string key,
                                            GestureActions actions) {
  mappings_[std::move(key)] = std::move(actions);
}

void SmartGestureMapping::Remove(std::string_view key) {
  mappings_.erase(key);
}

void SmartGestureMapping::ResetToDefaults() {
  mappings_ = DefaultSmartGestureMappings();
}

}  // namespace gestures
