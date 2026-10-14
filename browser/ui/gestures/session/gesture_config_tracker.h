// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_GESTURES_SESSION_GESTURE_CONFIG_TRACKER_H_
#define BRAVE_BROWSER_UI_GESTURES_SESSION_GESTURE_CONFIG_TRACKER_H_

#include <string_view>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/gestures/core/common/custom/custom_gesture_recognizer.h"
#include "brave/components/gestures/core/common/directional/directional_gesture_mapping.h"
#include "brave/components/gestures/core/common/smart/smart_gesture_mapping.h"
#include "components/prefs/pref_change_registrar.h"

class PrefService;

namespace gestures {

// Loads directional/custom/smart gesture settings from `prefs` and keeps
// them in sync with each relevant pref change.
class GestureConfigTracker final {
 public:
  // `prefs` is not owned, and must outlive this object; `GestureController`
  // gets it from its own tab's `BrowserContext`, which outlives it.
  explicit GestureConfigTracker(PrefService& prefs);

  GestureConfigTracker(const GestureConfigTracker&) = delete;
  GestureConfigTracker& operator=(const GestureConfigTracker&) = delete;

  ~GestureConfigTracker();

  // Whether gestures may fire on `scheme`/`host` (both already lowercase),
  // checked against the user's disallowed-sites pref.
  bool IsAllowedForHost(std::string_view scheme, std::string_view host) const;

  const DirectionalGestureMapping& directional_gesture_mapping() const {
    return directional_gesture_mapping_;
  }

  const CustomGestureRecognizer& custom_gesture_recognizer() const {
    return custom_gesture_recognizer_;
  }

  const SmartGestureMapping& smart_gesture_mapping() const {
    return smart_gesture_mapping_;
  }

 private:
  void LoadDirectionalGestures();
  void LoadCustomGestures();
  void LoadSmartGestures();

  const raw_ref<PrefService> prefs_;
  PrefChangeRegistrar pref_change_registrar_;

  DirectionalGestureMapping directional_gesture_mapping_;
  CustomGestureRecognizer custom_gesture_recognizer_;
  SmartGestureMapping smart_gesture_mapping_;

  base::WeakPtrFactory<GestureConfigTracker> weak_factory_{this};
};

}  // namespace gestures

#endif  // BRAVE_BROWSER_UI_GESTURES_SESSION_GESTURE_CONFIG_TRACKER_H_
