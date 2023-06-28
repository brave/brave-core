/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/android_data_files_installer.h"

#include <string>
#include <vector>

#include "base/containers/contains.h"
#include "base/functional/callback.h"
#include "base/functional/callback_forward.h"
#include "base/logging.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "chrome/browser/browser_process.h"  // TODO(AlexeyBarabash): forbidden include
#include "components/component_updater/component_updater_service.h"
#include "components/update_client/update_client_errors.h"

namespace brave_wallet {

namespace {
// TODO(AlexeyBarabash): code duplication
const char kComponentId[] = "bbckkcdiepaecefgfnibemejliemjnio";
}  // namespace

bool IsBraveWalletDataFilesComponentRegistered() {
  std::vector<std::string> registered_ids =
      g_browser_process->component_updater()->GetComponentIDs();
  return base::Contains(registered_ids, std::string(kComponentId));
}

bool IsBraveWalletDataFilesComponentInstalled() {
  std::vector<component_updater::ComponentInfo> component_infos =
      g_browser_process->component_updater()->GetComponents();

  auto it = std::find_if(component_infos.begin(), component_infos.end(),
                         [](const component_updater::ComponentInfo& ci) {
                           return ci.id == kComponentId;
                         });

  return it != component_infos.end();
}

}  // namespace brave_wallet
