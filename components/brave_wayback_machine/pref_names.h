/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WAYBACK_MACHINE_PREF_NAMES_H_
#define BRAVE_COMPONENTS_BRAVE_WAYBACK_MACHINE_PREF_NAMES_H_

inline constexpr char kBraveWaybackMachineEnabled[] =
    "brave.wayback_machine_enabled";

// Whether to automatically check the Wayback Machine for an archived version
// of a missing page. Only applies when kBraveWaybackMachineEnabled is true.
inline constexpr char kBraveWaybackMachineAutoCheckEnabled[] =
    "brave.wayback_machine_auto_check_enabled";


#endif  // BRAVE_COMPONENTS_BRAVE_WAYBACK_MACHINE_PREF_NAMES_H_
