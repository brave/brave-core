/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/scoped_txs_update.h"

#include "brave/components/brave_wallet/browser/tx_storage_delegate.h"

namespace brave_wallet {

ScopedTxsUpdate::ScopedTxsUpdate(TxStorageDelegate& delegate)
    : delegate_(delegate) {}

ScopedTxsUpdate::~ScopedTxsUpdate() {
  delegate_->ScheduleWrite();
}

base::Value::Dict& ScopedTxsUpdate::Get() {
  return delegate_->GetTxs();
}

}  // namespace brave_wallet
