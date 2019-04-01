/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ledger/bat_ledger_service_impl.h"

#include <utility>

#include "bat/ledger/ledger.h"
#include "brave/components/services/bat_ledger/bat_ledger_impl.h"
#include "mojo/public/cpp/bindings/strong_associated_binding.h"

namespace {

bool testing() {
  return ledger::is_testing;
}

}

namespace bat_ledger {

BatLedgerServiceImpl::BatLedgerServiceImpl(
    std::unique_ptr<service_manager::ServiceContextRef> service_ref)
  : service_ref_(std::move(service_ref)),
    initialized_(false) {
}

BatLedgerServiceImpl::~BatLedgerServiceImpl() {
}

void BatLedgerServiceImpl::Create(
    mojom::BatLedgerClientAssociatedPtrInfo client_info,
    mojom::BatLedgerAssociatedRequest bat_ledger) {
  mojo::MakeStrongAssociatedBinding(
      std::make_unique<BatLedgerImpl>(std::move(client_info)),
                                      std::move(bat_ledger));
  initialized_ = true;
}

void BatLedgerServiceImpl::SetProduction(bool is_production) {
  DCHECK(!initialized_ || testing());
  ledger::is_production = is_production;
}

void BatLedgerServiceImpl::SetDebug(bool is_debug) {
  DCHECK(!initialized_ || testing());
  ledger::is_debug = is_debug;
}

void BatLedgerServiceImpl::SetReconcileTime(int32_t time) {
  DCHECK(!initialized_ || testing());
  ledger::reconcile_time = time;
}

void BatLedgerServiceImpl::SetShortRetries(bool short_retries) {
  DCHECK(!initialized_ || testing());
  ledger::short_retries = short_retries;
}

void BatLedgerServiceImpl::SetTesting() {
  ledger::is_testing = true;
}

void BatLedgerServiceImpl::GetProduction(GetProductionCallback callback) {
  std::move(callback).Run(ledger::is_production);
}

void BatLedgerServiceImpl::GetDebug(GetDebugCallback callback) {
  std::move(callback).Run(ledger::is_debug);
}

void BatLedgerServiceImpl::GetReconcileTime(GetReconcileTimeCallback callback) {
  std::move(callback).Run(ledger::reconcile_time);
}

void BatLedgerServiceImpl::GetShortRetries(GetShortRetriesCallback callback) {
  std::move(callback).Run(ledger::short_retries);
}

}  // namespace bat_ledger
