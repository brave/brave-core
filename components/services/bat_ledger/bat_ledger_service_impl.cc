/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ledger/bat_ledger_service_impl.h"

#include <utility>

#include "brave/components/services/bat_ledger/bat_ledger_impl.h"

namespace {

bool testing() {
  return ledger::is_testing;
}

}

namespace bat_ledger {

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

void BatLedgerServiceImpl::SetEnvironment(
    ledger::type::Environment environment) {
  DCHECK(!initialized_ || testing());
  ledger::_environment = environment;
}

void BatLedgerServiceImpl::SetDebug(bool is_debug) {
  DCHECK(!initialized_ || testing());
  ledger::is_debug = is_debug;
}

void BatLedgerServiceImpl::SetReconcileInterval(const int32_t interval) {
  DCHECK(!initialized_ || testing());
  ledger::reconcile_interval = interval;
}

void BatLedgerServiceImpl::SetRetryInterval(int32_t interval) {
  DCHECK(!initialized_ || testing());
  ledger::retry_interval = interval;
}

void BatLedgerServiceImpl::SetTesting() {
  ledger::is_testing = true;
}

void BatLedgerServiceImpl::GetEnvironment(GetEnvironmentCallback callback) {
  std::move(callback).Run(ledger::_environment);
}

void BatLedgerServiceImpl::GetDebug(GetDebugCallback callback) {
  std::move(callback).Run(ledger::is_debug);
}

void BatLedgerServiceImpl::GetReconcileInterval(
    GetReconcileIntervalCallback callback) {
  std::move(callback).Run(ledger::reconcile_interval);
}

void BatLedgerServiceImpl::GetRetryInterval(GetRetryIntervalCallback callback) {
  std::move(callback).Run(ledger::retry_interval);
}

}  // namespace bat_ledger
