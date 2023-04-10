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
    : receiver_(this, std::move(receiver)) {
  VLOG(0) << "Constructor";

  ledgers_.set_disconnect_handler(base::BindRepeating(
      &LedgerFactoryImpl::OnDisconnect, base::Unretained(this)));
}

LedgerFactoryImpl::~LedgerFactoryImpl() {
  VLOG(0) << "Destructor";
}

void LedgerFactoryImpl::CreateLedger(
    const base::FilePath& profile_path,
    mojo::PendingAssociatedReceiver<mojom::Ledger> ledger_receiver,
    mojo::PendingAssociatedRemote<mojom::LedgerClient> ledger_client_remote,
    CreateLedgerCallback callback) {
  ledgers_.Add(std::make_unique<LedgerImpl>(std::move(ledger_client_remote)),
               std::move(ledger_receiver), profile_path);

  VLOG(0) << "Added " << profile_path;
  VLOG(0) << "Number of ledgers: " << ledgers_.size();

  std::move(callback).Run();
}

void LedgerFactoryImpl::OnDisconnect() {
  VLOG(0) << "Removed " << ledgers_.current_context();
  VLOG(0) << "Number of ledgers: " << ledgers_.size();

  if (ledgers_.empty()) {
    receiver_.reset();
  }
}

}  // namespace ledger
