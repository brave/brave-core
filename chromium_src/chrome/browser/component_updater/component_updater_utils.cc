/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/chrome/browser/component_updater/component_updater_utils.cc"

#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"

namespace component_updater {

void BraveOnDemandUpdate(const std::string& component_id) {
  brave_component_updater::BraveOnDemandUpdater::GetInstance()->OnDemandUpdate(
      component_id);
}

}  // namespace component_updater
