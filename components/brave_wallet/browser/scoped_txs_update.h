/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SCOPED_TXS_UPDATE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SCOPED_TXS_UPDATE_H_

#include "base/memory/raw_ref.h"
#include "base/values.h"

namespace brave_wallet {

class TxStorageDelegate;

class ScopedTxsUpdate {
 public:
  explicit ScopedTxsUpdate(TxStorageDelegate& delegate);
  ScopedTxsUpdate(const ScopedTxsUpdate&) = delete;
  ScopedTxsUpdate& operator=(const ScopedTxsUpdate&) = delete;
  virtual ~ScopedTxsUpdate();

  base::Value::Dict& Get();

  base::Value::Dict& operator*() { return Get(); }

  base::Value::Dict* operator->() { return &Get(); }

 private:
  const raw_ref<TxStorageDelegate> delegate_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SCOPED_TXS_UPDATE_H_
