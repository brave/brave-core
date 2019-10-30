/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WAYBACK_MACHINE_BROWSER_WAYBACK_MACHINE_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_WAYBACK_MACHINE_BROWSER_WAYBACK_MACHINE_UTIL_H_

namespace content {
class BrowserContext;
}

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace brave_wayback_machine {

bool IsBraveWaybackMachineEnabled(content::BrowserContext* browser_context);
bool IsBraveWaybackMachinePrefEnabled(content::BrowserContext* browser_context);
void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

}  // brave_wayback_machine

#endif  // BRAVE_COMPONENTS_BRAVE_WAYBACK_MACHINE_BROWSER_WAYBACK_MACHINE_UTIL_H_
