/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ledger/bat_ledger_service_impl.h"

#include "bat/ledger/ledger.h"
#include "brave/components/services/bat_ledger/bat_ledger_impl.h"
#include "mojo/public/cpp/bindings/strong_associated_binding.h"

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

void BatLedgerServiceImpl::SetProduction(bool isProduction) {
  DCHECK(!initialized_);
  ledger::is_production = isProduction;
}

void BatLedgerServiceImpl::SetReconcileTime(int32_t time) {
  DCHECK(!initialized_);
  ledger::reconcile_time = time;
}

void BatLedgerServiceImpl::SetShortRetries(bool short_retries) {
  DCHECK(!initialized_);
  ledger::short_retries = short_retries;
}

} // namespace bat_ledger
