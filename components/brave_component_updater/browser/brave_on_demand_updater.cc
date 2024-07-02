/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"

#include <string>
#include <utility>

#include "base/check_is_test.h"
#include "base/functional/callback.h"  // IWYU pragma: keep
#include "base/no_destructor.h"

namespace brave_component_updater {

BraveOnDemandUpdater* BraveOnDemandUpdater::GetInstance() {
  static base::NoDestructor<BraveOnDemandUpdater> instance;
  return instance.get();
}

BraveOnDemandUpdater::BraveOnDemandUpdater() = default;

BraveOnDemandUpdater::~BraveOnDemandUpdater() = default;

component_updater::OnDemandUpdater*
BraveOnDemandUpdater::RegisterOnDemandUpdater(
    component_updater::OnDemandUpdater* on_demand_updater) {
  if (!on_demand_updater) {
    CHECK_IS_TEST();
  }
  return std::exchange(on_demand_updater_, on_demand_updater);
}

void BraveOnDemandUpdater::OnDemandInstall(
    const std::string& id,
    component_updater::Callback callback) {
  CHECK(on_demand_updater_);
  on_demand_updater_->OnDemandInstall(id, std::move(callback));
}

void BraveOnDemandUpdater::OnDemandUpdate(
    const std::string& id,
    component_updater::OnDemandUpdater::Priority priority,
    component_updater::Callback callback) {
  CHECK(on_demand_updater_);
  on_demand_updater_->OnDemandUpdate(id, priority, std::move(callback));
}

void BraveOnDemandUpdater::OnDemandUpdate(
    const std::vector<std::string>& ids,
    component_updater::OnDemandUpdater::Priority priority,
    component_updater::Callback callback) {
  CHECK(on_demand_updater_);
  on_demand_updater_->OnDemandUpdate(ids, priority, std::move(callback));
}

}  // namespace brave_component_updater
