/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define RegisterWidevineCdmComponent RegisterWidevineCdmComponent_ChromiumImpl
#include "../../../../../chrome/browser/component_updater/widevine_cdm_component_installer.cc"  // NOLINT
#undef RegisterWidevineCdmComponent

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/common/extensions/extension_constants.h"
#include "chrome/browser/ui/webui/components_ui.h"
#include "components/component_updater/component_updater_service.h"
#include "third_party/widevine/cdm/buildflags.h"

#if BUILDFLAG(ENABLE_WIDEVINE_CDM_COMPONENT)
#include "brave/browser/widevine/widevine_utils.h"
#endif

namespace component_updater {

#if BUILDFLAG(ENABLE_WIDEVINE_CDM_COMPONENT)

void OnWidevineRegistered() {
  ComponentsUI::OnDemandUpdate(widevine_extension_id);
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
  if (IsWidevineOptedIn())
    RegisterAndInstallWidevine();
#endif  // defined(ENABLE_WIDEVINE_CDM_COMPONENT)
}

}  // namespace component_updater
