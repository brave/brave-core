/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_SELF_OWNED_LEDGER_RECEIVER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_SELF_OWNED_LEDGER_RECEIVER_H_

#include "base/functional/callback_forward.h"
#include "brave/components/brave_rewards/common/mojom/bat_ledger.mojom.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "mojo/public/cpp/bindings/pending_associated_remote.h"

namespace brave_rewards::internal {

class SelfOwnedLedgerReceiver {
 public:
  static void Create(mojo::PendingAssociatedRemote<mojom::LedgerClient> remote,
                     mojo::PendingAssociatedReceiver<mojom::Ledger> receiver,
                     base::OnceClosure disconnect_handler);

  SelfOwnedLedgerReceiver(const SelfOwnedLedgerReceiver&) = delete;
  SelfOwnedLedgerReceiver& operator=(const SelfOwnedLedgerReceiver&) = delete;

 private:
  void DeleteThis();

  SelfOwnedLedgerReceiver(
      mojo::PendingAssociatedRemote<mojom::LedgerClient> remote,
      mojo::PendingAssociatedReceiver<mojom::Ledger> receiver,
      base::OnceClosure disconnect_handler);

  ~SelfOwnedLedgerReceiver();

  mojo::AssociatedReceiver<mojom::Ledger> receiver_;
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_SELF_OWNED_LEDGER_RECEIVER_H_
