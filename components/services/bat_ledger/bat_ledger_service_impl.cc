/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ledger/bat_ledger_service_impl.h"

#include <memory>
#include <utility>

#include "brave/components/services/bat_ledger/bat_ledger_impl.h"

namespace {

bool testing() {
  return brave_rewards::core::is_testing;
}

}

namespace brave_rewards {

BatLedgerServiceImpl::BatLedgerServiceImpl(
    mojo::PendingReceiver<mojom::BatLedgerService> receiver)
    : receiver_(this, std::move(receiver)),
    initialized_(false) {}

BatLedgerServiceImpl::~BatLedgerServiceImpl() = default;

void BatLedgerServiceImpl::Create(
    mojo::PendingAssociatedRemote<mojom::BatLedgerClient> client_info,
    mojo::PendingAssociatedReceiver<mojom::BatLedger> bat_ledger,
    CreateCallback callback) {
  associated_receivers_.Add(
      std::make_unique<BatLedgerImpl>(std::move(client_info)),
      std::move(bat_ledger));
  initialized_ = true;
  std::move(callback).Run();
}

void BatLedgerServiceImpl::SetEnvironment(mojom::Environment environment) {
  DCHECK(!initialized_ || testing());
  core::_environment = environment;
}

void BatLedgerServiceImpl::SetDebug(bool is_debug) {
  DCHECK(!initialized_ || testing());
  core::is_debug = is_debug;
}

void BatLedgerServiceImpl::SetReconcileInterval(const int32_t interval) {
  DCHECK(!initialized_ || testing());
  core::reconcile_interval = interval;
}

void BatLedgerServiceImpl::SetRetryInterval(int32_t interval) {
  DCHECK(!initialized_ || testing());
  core::retry_interval = interval;
}

void BatLedgerServiceImpl::SetTesting() {
  core::is_testing = true;
}

void BatLedgerServiceImpl::SetStateMigrationTargetVersionForTesting(
    int32_t version) {
  core::state_migration_target_version_for_testing = version;
}

void BatLedgerServiceImpl::GetEnvironment(GetEnvironmentCallback callback) {
  std::move(callback).Run(core::_environment);
}

void BatLedgerServiceImpl::GetDebug(GetDebugCallback callback) {
  std::move(callback).Run(core::is_debug);
}

void BatLedgerServiceImpl::GetReconcileInterval(
    GetReconcileIntervalCallback callback) {
  std::move(callback).Run(core::reconcile_interval);
}

void BatLedgerServiceImpl::GetRetryInterval(GetRetryIntervalCallback callback) {
  std::move(callback).Run(core::retry_interval);
}

}  // namespace brave_rewards
