/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/component_updater/component_updater_service.h"

#include "brave/browser/component_updater/brave_crx_update_service.h"
#include "components/update_client/crx_downloader.h"
#include "components/update_client/ping_manager.h"
#include "components/update_client/update_checker.h"
#include "components/update_client/update_client_internal.h"

#define ComponentUpdateServiceFactory ComponentUpdateServiceFactory_ChromiumImpl
#include "../../../../components/component_updater/component_updater_service.cc"  // NOLINT
#undef ComponentUpdateServiceFactory

using update_client::CrxDownloader;
using update_client::PingManager;
using update_client::UpdateChecker;
using update_client::UpdateClientImpl;

namespace component_updater {

std::unique_ptr<ComponentUpdateService> ComponentUpdateServiceFactory(
    scoped_refptr<Configurator> config,
    std::unique_ptr<UpdateScheduler> scheduler) {
  DCHECK(config);
  DCHECK(scheduler);
  auto update_client = base::MakeRefCounted<UpdateClientImpl>(
      config,
      base::MakeRefCounted<PingManager>(config),
      &UpdateChecker::Create,
      &CrxDownloader::Create);
  return std::make_unique<BraveCrxUpdateService>(
      config, std::move(scheduler), std::move(update_client));
}

}  // namespace component_updater
