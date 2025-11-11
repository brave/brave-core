/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/component_updater/registration.h"
#include "base/functional/bind.h"

#define RegisterComponentsForUpdate RegisterComponentsForUpdate_ChromiumImpl

#include <chrome/browser/component_updater/registration.cc>

#undef RegisterComponentsForUpdate

#include "brave/browser/brave_browser_process.h"
#include "brave/components/ai_chat/core/browser/local_models_updater.h"
#include "brave/components/brave_user_agent/browser/brave_user_agent_component_installer.h"
#include "brave/components/brave_wallet/browser/wallet_data_files_installer.h"
#include "brave/components/p3a/managed/component_installer.h"
#include "brave/components/p3a/p3a_service.h"
#include "brave/components/psst/buildflags/buildflags.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/component_updater/component_updater_utils.h"

#if BUILDFLAG(ENABLE_PSST)
#include "brave/components/psst/browser/core/psst_component_installer.h"
#endif

#if BUILDFLAG(IS_ANDROID)
#include "chrome/browser/component_updater/zxcvbn_data_component_installer.h"
#endif  // BUILDFLAG(IS_ANDROID)

namespace component_updater {

void RegisterComponentsForUpdate() {
  RegisterComponentsForUpdate_ChromiumImpl();
  ComponentUpdateService* cus = g_browser_process->component_updater();
  brave_wallet::WalletDataFilesInstaller::GetInstance()
      .MaybeRegisterWalletDataFilesComponent(cus,
                                             g_browser_process->local_state());
#if BUILDFLAG(ENABLE_PSST)
  psst::RegisterPsstComponent(cus);
#endif
  p3a::MaybeToggleP3AComponent(cus, g_brave_browser_process->p3a_service());
#if BUILDFLAG(IS_ANDROID)
  // Currently behind !BUILDFLAG(IS_ANDROID) in upstream.
  RegisterZxcvbnDataComponent(cus);
#endif  // BUILDFLAG(IS_ANDROID)
  brave_user_agent::RegisterBraveUserAgentComponent(cus);
}

}  // namespace component_updater
