// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_UI_PREF_NAMES_H_
#define BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_UI_PREF_NAMES_H_

namespace containers::prefs {

// Controls whether Containers feature (menus, management UI) is available;
// existing container tabs are not affected when the pref is disabled.
inline constexpr char kContainersEnabled[] = "brave.containers.enabled";

}  // namespace containers::prefs

#endif  // BRAVE_COMPONENTS_CONTAINERS_CORE_BROWSER_UI_PREF_NAMES_H_
