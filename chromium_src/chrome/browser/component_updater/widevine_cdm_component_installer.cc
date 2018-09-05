/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define RegisterWidevineCdmComponent RegisterWidevineCdmComponent_ChromiumImpl
#include "../../../../../chrome/browser/component_updater/widevine_cdm_component_installer.cc"
#undef RegisterWidevineCdmComponent

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/prefs/pref_service.h"

namespace component_updater {

// Do nothing, we will register if the user opts in!
void RegisterWidevineCdmComponent(ComponentUpdateService* cus) {

  PrefService* prefs = ProfileManager::GetActiveUserProfile()->GetPrefs();
  bool widevine_opted_in =
      prefs->GetBoolean(kWidevineOptedIn);
  if (widevine_opted_in) {
    RegisterWidevineCdmComponent_ChromiumImpl(
        g_brave_browser_process->component_updater());
  }
}

}  // namespace component_updater

