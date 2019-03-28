/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_COMPONENT_UPDATER_BRAVE_CRX_UPDATE_SERVICE_H_
#define BRAVE_BROWSER_COMPONENT_UPDATER_BRAVE_CRX_UPDATE_SERVICE_H_

#include <memory>

#include "components/component_updater/component_updater_service.h"
#include "components/component_updater/component_updater_service_internal.h"

namespace component_updater {

using CrxInstaller = update_client::CrxInstaller;
using UpdateClient = update_client::UpdateClient;

class BraveCrxUpdateService : public CrxUpdateService {
 public:
  using CrxUpdateService::CrxUpdateService;
  BraveCrxUpdateService(scoped_refptr<Configurator> config,
                        std::unique_ptr<UpdateScheduler> scheduler,
                        scoped_refptr<UpdateClient> update_client);

  bool RegisterComponent(const CrxComponent& component) override;
  ~BraveCrxUpdateService() override {}

 private:
  bool CheckForUpdates(UpdateScheduler::OnFinishedCallback on_finished);
  void Start();

  DISALLOW_COPY_AND_ASSIGN(BraveCrxUpdateService);
};
}  // namespace component_updater

#endif  // BRAVE_BROWSER_COMPONENT_UPDATER_BRAVE_CRX_UPDATE_SERVICE_H_
