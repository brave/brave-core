/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define RegisterWidevineCdmComponent RegisterWidevineCdmComponent_ChromiumImpl
#include "../../../../../chrome/browser/component_updater/widevine_cdm_component_installer.cc"  // NOLINT
#undef RegisterWidevineCdmComponent

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/widevine/widevine_utils.h"
#include "chrome/browser/component_updater/component_updater_utils.h"
#include "components/component_updater/component_updater_service.h"
#include "extensions/common/constants.h"

namespace component_updater {

void OnWidevineRegistered() {
  component_updater::BraveOnDemandUpdate(widevine_extension_id);
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

// Do nothing unless the user opts in!
void RegisterWidevineCdmComponent(ComponentUpdateService* cus) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  if (IsWidevineOptedIn())
    RegisterAndInstallWidevine();
}

}  // namespace component_updater
