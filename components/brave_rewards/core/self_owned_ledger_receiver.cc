/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/self_owned_ledger_receiver.h"

#include <utility>

#include "base/functional/callback.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"

// In case of a disconnection (regardless of the cause), the
// mojo::AssociatedReceiver<mojom::Ledger>'s disconnect handler is invoked,
// which does two things:
//   1. tears down the SelfOwnedLedgerReceiver:
//   The mojo::AssociatedReceiver<mojom::Ledger> gets destroyed - this by itself
//   (even without disconnection) means that no method call can make its
//   way to LedgerImpl.
//
//   2. calls the disconnect handler passed to the SelfOwnedLedgerReceiver:
//   This will call LedgerFactoryImpl::LedgerRemovedCallback, which
//   removes/stops the ledger thread for the profile on behalf of which it's
//   running. Pending tasks queued on the thread's message loop will run to
//   completion. Consequently, it should always be safe to use
//   base::Unretained<>() for LedgerImpl (or any of its subobjects), as TLS is
//   only deallocated when the thread ends.

namespace brave_rewards::internal {

void SelfOwnedLedgerReceiver::Create(
    mojo::PendingAssociatedRemote<mojom::LedgerClient> remote,
    mojo::PendingAssociatedReceiver<mojom::Ledger> receiver,
    base::OnceClosure disconnect_handler) {
  new SelfOwnedLedgerReceiver(std::move(remote), std::move(receiver),
                              std::move(disconnect_handler));
}

void SelfOwnedLedgerReceiver::DeleteThis() {
  delete this;
}

SelfOwnedLedgerReceiver::SelfOwnedLedgerReceiver(
    mojo::PendingAssociatedRemote<mojom::LedgerClient> remote,
    mojo::PendingAssociatedReceiver<mojom::Ledger> receiver,
    base::OnceClosure disconnect_handler)
    : receiver_(&ledger(std::move(remote)), std::move(receiver)) {
  receiver_.set_disconnect_handler(
      base::BindOnce(&SelfOwnedLedgerReceiver::DeleteThis,
                     base::Unretained(this))
          .Then(std::move(disconnect_handler)));
}

SelfOwnedLedgerReceiver::~SelfOwnedLedgerReceiver() = default;

}  // namespace brave_rewards::internal
