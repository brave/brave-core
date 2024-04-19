/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_component_updater/browser/mock_on_demand_updater.h"

#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"

namespace brave_component_updater {

MockOnDemandUpdater::MockOnDemandUpdater() {
  prev_on_demand_updater_ =
      BraveOnDemandUpdater::GetInstance()->RegisterOnDemandUpdater(this);
}

MockOnDemandUpdater::~MockOnDemandUpdater() {
  BraveOnDemandUpdater::GetInstance()->RegisterOnDemandUpdater(
      prev_on_demand_updater_);
}

}  // namespace brave_component_updater
