/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"

#include <string>

#include "base/memory/singleton.h"

namespace brave_component_updater {

BraveOnDemandUpdater* BraveOnDemandUpdater::GetInstance() {
  return base::Singleton<BraveOnDemandUpdater>::get();
}

BraveOnDemandUpdater::BraveOnDemandUpdater() {}

BraveOnDemandUpdater::~BraveOnDemandUpdater() {}

void BraveOnDemandUpdater::OnDemandUpdate(const std::string& id) {
  DCHECK(!on_demand_update_callback_.is_null());
  on_demand_update_callback_.Run(id);
}

void BraveOnDemandUpdater::RegisterOnDemandUpdateCallback(Callback callback) {
  on_demand_update_callback_ = callback;
}


}  // namespace brave_component_updater
