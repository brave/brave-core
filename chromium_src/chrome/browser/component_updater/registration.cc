/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/component_updater/registration.h"
#include "base/functional/bind.h"
#include "brave/components/widevine/static_buildflags.h"
#include "chrome/browser/component_updater/widevine_cdm_component_installer.h"

#define RegisterComponentsForUpdate RegisterComponentsForUpdate_ChromiumImpl

#if BUILDFLAG(WIDEVINE_ARM64_DLL_FIX)
#define RegisterWidevineCdmComponent(cus) \
  RegisterWidevineCdmComponent(cus,       \
                               g_browser_process->shared_url_loader_factory())
#else
#define RegisterWidevineCdmComponent(cus) RegisterWidevineCdmComponent(cus)
#endif

#include "src/chrome/browser/component_updater/registration.cc"

#undef RegisterWidevineCdmComponent
#undef RegisterComponentsForUpdate

#include "brave/components/brave_wallet/browser/wallet_data_files_installer.h"
#include "brave/components/psst/browser/core/psst_component_installer.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/component_updater/component_updater_utils.h"

namespace component_updater {

void RegisterComponentsForUpdate() {
  RegisterComponentsForUpdate_ChromiumImpl();
  ComponentUpdateService* cus = g_browser_process->component_updater();
  brave_wallet::WalletDataFilesInstaller::GetInstance()
      .MaybeRegisterWalletDataFilesComponent(cus,
                                             g_browser_process->local_state());
  psst::RegisterPsstComponent(
      cus, base::BindOnce(&component_updater::BraveOnDemandUpdate));
}

}  // namespace component_updater
