/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/services/ledger/ledger_factory_impl.h"

#include <memory>
#include <utility>

#include "brave/components/brave_rewards/core/ledger_impl.h"

namespace ledger {

LedgerFactoryImpl::LedgerFactoryImpl(
    mojo::PendingReceiver<mojom::LedgerFactory> receiver)
    : receiver_(this, std::move(receiver)) {}

LedgerFactoryImpl::~LedgerFactoryImpl() = default;

void LedgerFactoryImpl::CreateLedger(
    mojo::PendingAssociatedReceiver<mojom::Ledger> ledger_receiver,
    mojo::PendingAssociatedRemote<mojom::LedgerClient> ledger_client_remote,
    CreateLedgerCallback callback) {
  if (!ledger_) {
    ledger_ = mojo::MakeSelfOwnedAssociatedReceiver(
        std::make_unique<LedgerImpl>(std::move(ledger_client_remote)),
        std::move(ledger_receiver));
  }

  std::move(callback).Run();
}

}  // namespace ledger
