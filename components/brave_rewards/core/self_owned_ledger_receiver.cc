/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/self_owned_ledger_receiver.h"

#include <utility>

#include "base/functional/callback.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"

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
