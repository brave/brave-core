/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define RegisterWidevineCdmComponent RegisterWidevineCdmComponent_ChromiumImpl
#include "../../../../../chrome/browser/component_updater/widevine_cdm_component_installer.cc"
#undef RegisterWidevineCdmComponent

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/extensions/brave_component_extension.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "brave/common/extensions/extension_constants.h"
#include "components/component_updater/component_updater_service.h"
#include "components/prefs/pref_service.h"
#include "third_party/widevine/cdm/buildflags.h"

namespace component_updater {

#if BUILDFLAG(ENABLE_WIDEVINE_CDM_COMPONENT)


void OnWidevineRegistered() {
  ComponentsUI demand_updater;
  // This weird looking call is ok, it is just like this to not need
  // to patch for friend access.
  demand_updater.OnDemandUpdate(g_browser_process->component_updater(),
      widevine_extension_id);
}

void RegisterAndInstallWidevine() {
  // This code is similar to RegisterWidevineCdmComponent_ChromiumImpl
  // but that ignores the callback, and we handle it so we can force
  // an on demand update.
  auto installer = base::MakeRefCounted<component_updater::ComponentInstaller>(
      std::make_unique<WidevineCdmComponentInstallerPolicy>());
  installer->Register(g_browser_process->component_updater(),
      base::Bind(&OnWidevineRegistered));
}

#endif

// Do nothing unless the user opts in!
void RegisterWidevineCdmComponent(ComponentUpdateService* cus) {
#if BUILDFLAG(ENABLE_WIDEVINE_CDM_COMPONENT)
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  PrefService* prefs = ProfileManager::GetActiveUserProfile()->GetPrefs();
  bool widevine_opted_in =
      prefs->GetBoolean(kWidevineOptedIn);
  if (widevine_opted_in) {
    RegisterAndInstallWidevine();
  }
#endif  // defined(ENABLE_WIDEVINE_CDM_COMPONENT)
}

}  // namespace component_updater
