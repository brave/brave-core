/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_ACTION_CONTEXT_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_ACTION_CONTEXT_H_

#include <string>

#include "base/memory/raw_ref.h"
#include "base/threading/sequence_bound.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/buildflags.h"

namespace brave_wallet {

class ZCashRpc;
class OrchardSyncState;

// Basic context required by most orchard-related operations.
struct ZCashActionContext {
  ZCashActionContext(ZCashRpc& zcash_rpc,
#if BUILDFLAG(ENABLE_ORCHARD)
                     base::SequenceBound<OrchardSyncState>& sync_state,
#endif  // BUILDFLAG(ENABLE_ORCHARD)
                     const mojom::AccountIdPtr& account_id,
                     const std::string& chain_id);
  ~ZCashActionContext();
  raw_ref<ZCashRpc> zcash_rpc;
#if BUILDFLAG(ENABLE_ORCHARD)
  raw_ref<base::SequenceBound<OrchardSyncState>> sync_state;
#endif  // BUILDFLAG(ENABLE_ORCHARD)
  ZCashActionContext(ZCashActionContext&) = delete;
  ZCashActionContext& operator=(ZCashActionContext&) = delete;
  ZCashActionContext& operator=(ZCashActionContext&&);
  ZCashActionContext(ZCashActionContext&&);
  mojom::AccountIdPtr account_id;
  std::string chain_id;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_ACTION_CONTEXT_H_
