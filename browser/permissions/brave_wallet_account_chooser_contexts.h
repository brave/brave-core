/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PERMISSIONS_BRAVE_WALLET_ACCOUNT_CHOOSER_CONTEXTS_H_
#define BRAVE_BROWSER_PERMISSIONS_BRAVE_WALLET_ACCOUNT_CHOOSER_CONTEXTS_H_

#include <map>
#include <memory>

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/permissions/contexts/brave_wallet_account_chooser_context.h"
#include "components/keyed_service/core/keyed_service.h"

namespace brave_wallet {

// Owns one BraveWalletAccountChooserContext per coin. A single instance cannot
// serve every coin: ObjectPermissionContextBase rewrites an origin's whole
// object list on save, so each coin needs its own data content settings type
// and therefore its own context.
class BraveWalletAccountChooserContexts : public KeyedService {
 public:
  using ContextMap =
      std::map<mojom::CoinType,
               std::unique_ptr<permissions::BraveWalletAccountChooserContext>>;

  explicit BraveWalletAccountChooserContexts(ContextMap contexts);

  BraveWalletAccountChooserContexts(const BraveWalletAccountChooserContexts&) =
      delete;
  BraveWalletAccountChooserContexts& operator=(
      const BraveWalletAccountChooserContexts&) = delete;

  ~BraveWalletAccountChooserContexts() override;

  // Returns null for coins that have no wallet permission.
  permissions::BraveWalletAccountChooserContext* Get(mojom::CoinType coin);

  // KeyedService:
  void Shutdown() override;

 private:
  ContextMap contexts_;
};

}  // namespace brave_wallet

#endif  // BRAVE_BROWSER_PERMISSIONS_BRAVE_WALLET_ACCOUNT_CHOOSER_CONTEXTS_H_
