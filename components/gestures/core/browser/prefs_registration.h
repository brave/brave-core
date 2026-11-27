// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_BROWSER_PREFS_REGISTRATION_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_BROWSER_PREFS_REGISTRATION_H_

#include "brave/components/gestures/core/browser/input/mouse/mouse_rocker_default_commands_info.h"
#include "brave/components/gestures/core/browser/input/mouse/mouse_wheel_default_commands_info.h"

namespace user_prefs {
class PrefRegistrySyncable;
}  // namespace user_prefs

namespace gestures {

// `mouse_rocker_defaults`/`mouse_wheel_defaults` are `IDC_*` ids,
// caller-supplied since this component can't depend on `IDC_*` headers.
void RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry,
    const MouseRockerDefaultCommandsInfo& mouse_rocker_defaults,
    const MouseWheelDefaultCommandsInfo& mouse_wheel_defaults);

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_BROWSER_PREFS_REGISTRATION_H_
