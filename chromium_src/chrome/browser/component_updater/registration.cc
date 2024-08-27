/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/component_updater/registration.h"
#include "base/functional/bind.h"

#define RegisterComponentsForUpdate RegisterComponentsForUpdate_ChromiumImpl

#include "src/chrome/browser/component_updater/registration.cc"

#undef RegisterComponentsForUpdate

#include "brave/components/ai_chat/core/browser/local_models_updater.h"
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
  psst::RegisterPsstComponent(cus);
  ai_chat::ManageLocalModelsComponentRegistration(cus);
}

}  // namespace component_updater
