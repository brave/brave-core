/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wayback_machine/browser/wayback_machine_util.h"

#include "brave/common/extensions/extension_constants.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "extensions/browser/extension_registry.h"

namespace brave_wayback_machine {

bool IsBraveWaybackMachineEnabled(content::BrowserContext* browser_context) {
  extensions::ExtensionRegistry* registry =
    extensions::ExtensionRegistry::Get(browser_context);
  return registry->enabled_extensions().Contains(brave_wayback_machine_extension_id);
}

bool IsBraveWaybackMachinePrefEnabled(content::BrowserContext* browser_context) {
  return Profile::FromBrowserContext(browser_context)->
      GetPrefs()->GetBoolean(kBraveWaybackMachineEnabled);
}

void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(kBraveWaybackMachineEnabled, true);
}

}  // namespace brave_wayback_machine
