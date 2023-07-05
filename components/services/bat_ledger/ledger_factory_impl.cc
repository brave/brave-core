/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ledger/ledger_factory_impl.h"

#include <memory>
#include <utility>

#include "brave/components/brave_rewards/core/ledger_impl.h"

namespace brave_rewards::internal {

LedgerFactoryImpl::LedgerFactoryImpl(
    mojo::PendingReceiver<mojom::LedgerFactory> receiver)
    : receiver_(this, std::move(receiver)) {}

LedgerFactoryImpl::~LedgerFactoryImpl() = default;

void LedgerFactoryImpl::CreateLedger(
    mojo::PendingAssociatedReceiver<mojom::Ledger> ledger_receiver,
    mojo::PendingAssociatedRemote<mojom::LedgerClient> ledger_client_remote,
    mojom::LedgerOptionsPtr options,
    CreateLedgerCallback callback) {
  if (!ledger_) {
    // Set global options for the current process. This must be done before
    // the `LedgerImpl` constructor is invoked so that subobjects see the
    // correct values.
    _environment = options->environment;
    is_testing = options->is_testing;
    is_debug = options->is_debug;
    state_migration_target_version_for_testing =
        options->state_migration_target_version_for_testing;
    reconcile_interval = options->reconcile_interval;
    retry_interval = options->retry_interval;

    ledger_ = mojo::MakeSelfOwnedAssociatedReceiver(
        std::make_unique<LedgerImpl>(std::move(ledger_client_remote)),
        std::move(ledger_receiver));
  }

  std::move(callback).Run();
}

}  // namespace brave_rewards::internal
