// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/web/brave_web_main_parts.h"

#include "base/command_line.h"
#include "base/path_service.h"
#include "base/strings/sys_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "brave/components/ai_chat/core/browser/local_models_updater.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "brave/components/brave_wallet/browser/wallet_data_files_installer.h"
#include "brave/ios/browser/application_context/brave_application_context_impl.h"
#include "components/component_updater/installer_policies/safety_tips_component_installer.h"
#include "ios/chrome/browser/application_context/model/application_context_impl.h"
#include "ios/chrome/browser/shared/model/paths/paths.h"
#include "ui/base/l10n/l10n_util_mac.h"
#include "ui/base/resource/resource_bundle.h"

namespace {
void RegisterComponentsForUpdate(
    component_updater::ComponentUpdateService* cus) {
  RegisterSafetyTipsComponent(cus);
  brave_wallet::WalletDataFilesInstaller::GetInstance()
      .MaybeRegisterWalletDataFilesComponent(
          cus, GetApplicationContext()->GetLocalState());
  ai_chat::ManageLocalModelsComponentRegistration(cus);
}
}  // namespace

BraveWebMainParts::BraveWebMainParts(
    const base::CommandLine& parsed_command_line)
    : IOSChromeMainParts(parsed_command_line) {}

BraveWebMainParts::~BraveWebMainParts() {}

void BraveWebMainParts::PreCreateMainMessageLoop() {
  IOSChromeMainParts::PreCreateMainMessageLoop();

  // Add Brave Resource Pack
  auto brave_pack_path = base::PathService::CheckedGet(base::DIR_ASSETS);
  brave_pack_path = brave_pack_path.AppendASCII("brave_resources.pak");
  ui::ResourceBundle::GetSharedInstance().AddDataPackFromPath(
      brave_pack_path, ui::kScaleFactorNone);
}

void BraveWebMainParts::PreMainMessageLoopRun() {
  IOSChromeMainParts::PreMainMessageLoopRun();

  // Setup Component Updater
  component_updater::ComponentUpdateService* cus =
      application_context_->GetComponentUpdateService();
  DCHECK(cus);
  brave_component_updater::BraveOnDemandUpdater::GetInstance()
      ->RegisterOnDemandUpdater(&cus->GetOnDemandUpdater());
  RegisterComponentsForUpdate(cus);

  static_cast<BraveApplicationContextImpl*>(application_context_.get())
      ->StartBraveServices();
}
