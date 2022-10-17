/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/component_updater/component_updater_utils.h"

#include "base/callback.h"
#include "components/component_updater/component_updater_service.h"
#include "ios/chrome/browser/application_context/application_context.h"

namespace component_updater {

void BraveOnDemandUpdate(const std::string& component_id) {
  component_updater::ComponentUpdateService* cus =
      GetApplicationContext()->GetComponentUpdateService();
  cus->GetOnDemandUpdater().OnDemandUpdate(
      component_id, component_updater::OnDemandUpdater::Priority::FOREGROUND,
      component_updater::Callback());
}

}  // namespace component_updater
