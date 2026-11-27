// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_BROWSER_SMART_SMART_GESTURE_MAPPING_PREF_LOADER_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_BROWSER_SMART_SMART_GESTURE_MAPPING_PREF_LOADER_H_

class PrefService;

namespace gestures {

class SmartGestureMapping;

// Resets `mapping` to its defaults, then overlays any user overrides stored
// in `prefs::kSmartGestures`.
void LoadSmartGestureMapping(PrefService& prefs, SmartGestureMapping& mapping);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_BROWSER_SMART_SMART_GESTURE_MAPPING_PREF_LOADER_H_
