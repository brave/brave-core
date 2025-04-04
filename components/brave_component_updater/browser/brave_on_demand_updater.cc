/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"

#include <string>
#include <thread>
#include <utility>

#include "base/check_is_test.h"
#include "base/functional/callback.h"  // IWYU pragma: keep
#include "base/logging.h"
#include "base/no_destructor.h"

namespace {
static std::string LogThreadId() {
  std::ostringstream ss;
  ss << std::this_thread::get_id();
  return ss.str();
}
}  // namespace

namespace brave_component_updater {

BraveOnDemandUpdater* BraveOnDemandUpdater::GetInstance() {
  static base::NoDestructor<BraveOnDemandUpdater> instance;
  instance->IsValid();
  return instance.get();
}
bool BraveOnDemandUpdater::IsValid() {
  // LOG(INFO) << "[PSST] BraveOnDemandUpdater::IsValid(): " <<
  // (on_demand_updater_ != nullptr);
  return on_demand_updater_ != nullptr;
}

BraveOnDemandUpdater::BraveOnDemandUpdater() = default;

BraveOnDemandUpdater::~BraveOnDemandUpdater() {
  LOG(INFO) << "[PSST] BraveOnDemandUpdater::~BraveOnDemandUpdater "
            << " PID:" << getpid() << " ThreadID:" << LogThreadId();
}

component_updater::OnDemandUpdater*
BraveOnDemandUpdater::RegisterOnDemandUpdater(
    component_updater::OnDemandUpdater* on_demand_updater) {
  if (!on_demand_updater) {
    CHECK_IS_TEST();
  }
  LOG(INFO) << "[PSST] BraveOnDemandUpdater::RegisterOnDemandUpdater";
  return std::exchange(on_demand_updater_, on_demand_updater);
}

void BraveOnDemandUpdater::EnsureInstalled(
    const std::string& id,
    component_updater::Callback callback) {
  LOG(INFO) << "[PSST] BraveOnDemandUpdater::EnsureInstalled "
            << " on_demand_updater_:" << on_demand_updater_
            << " PID:" << getpid() << " ThreadID:" << LogThreadId();
  CHECK(on_demand_updater_);
  on_demand_updater_->EnsureInstalled(id, std::move(callback));
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
