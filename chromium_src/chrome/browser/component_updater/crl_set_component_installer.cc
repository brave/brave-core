/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"

#define RegisterCRLSetComponent RegisterCRLSetComponent_ChromiumImpl
#include "src/chrome/browser/component_updater/crl_set_component_installer.cc"
#undef RegisterCRLSetComponent

#include "chrome/browser/browser_process.h"

#if !BUILDFLAG(IS_ANDROID)
#include "chrome/browser/component_updater/component_updater_utils.h"
#include "extensions/common/constants.h"
#endif

namespace component_updater {

void OnCRLSetRegistered() {
// https://github.com/brave/browser-android-tabs/issues/857
#if !BUILDFLAG(IS_ANDROID)
  brave_component_updater::BraveOnDemandUpdater::GetInstance()->OnDemandInstall(
      crl_set_extension_id);
#endif
}

void RegisterCRLSetComponent(ComponentUpdateService* cus) {
  auto installer = base::MakeRefCounted<component_updater::ComponentInstaller>(
      std::make_unique<CRLSetPolicy>());
  installer->Register(g_browser_process->component_updater(),
                      base::BindOnce(&OnCRLSetRegistered));
}

}  // namespace component_updater
