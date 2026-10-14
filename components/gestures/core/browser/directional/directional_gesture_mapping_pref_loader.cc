// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/browser/directional/directional_gesture_mapping_pref_loader.h"

#include <optional>
#include <utility>

#include "base/values.h"
#include "brave/components/gestures/core/browser/pref_names.h"
#include "brave/components/gestures/core/common/actions/gesture_action_info.h"
#include "brave/components/gestures/core/common/actions/gesture_action_parser.h"
#include "brave/components/gestures/core/common/directional/directional_gesture_mapping.h"
#include "components/prefs/pref_service.h"

namespace gestures {

void LoadDirectionalGestureMapping(PrefService& prefs,
                                   DirectionalGestureMapping& mapping) {
  mapping.ResetToDefaults();
  for (const auto [key, value] : prefs.GetDict(prefs::kDirectionalGestures)) {
    const base::ListValue* list = value.GetIfList();
    std::optional<GestureActions> actions =
        list ? MaybeParseGestureActions(*list) : std::nullopt;
    if (actions) {
      mapping.SetActions(key, std::move(*actions));
    }
  }
}

}  // namespace gestures
