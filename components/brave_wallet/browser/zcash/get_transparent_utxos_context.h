// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_GET_TRANSPARENT_UTXOS_CONTEXT_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_GET_TRANSPARENT_UTXOS_CONTEXT_H_

#include <string>

#include "base/functional/callback.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

class GetTransparentUtxosContext
    : public base::RefCountedThreadSafe<GetTransparentUtxosContext> {
 public:
  GetTransparentUtxosContext();
  using GetUtxosCallback = ZCashWalletService::GetUtxosCallback;

  std::set<std::string> addresses;
  ZCashWalletService::UtxoMap utxos;
  std::optional<std::string> error;
  GetUtxosCallback callback;

  bool ShouldRespond() { return callback && (error || addresses.empty()); }

  void SetError(const std::string& error_string) { error = error_string; }

 protected:
  friend class base::RefCountedThreadSafe<GetTransparentUtxosContext>;

  virtual ~GetTransparentUtxosContext();
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_GET_TRANSPARENT_UTXOS_CONTEXT_H_
