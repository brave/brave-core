/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_action_context.h"

#include "brave/components/brave_wallet/common/common_utils.h"

namespace brave_wallet {

ZCashActionContext::ZCashActionContext(
    ZCashRpc& zcash_rpc,
#if BUILDFLAG(ENABLE_ORCHARD)
    const std::optional<OrchardAddrRawPart>& account_internal_addr,
    base::SequenceBound<OrchardSyncState>& sync_state,
#endif  // BUILDFLAG(ENABLE_ORCHARD)
    const mojom::AccountIdPtr& account_id)
    : zcash_rpc(zcash_rpc),
#if BUILDFLAG(ENABLE_ORCHARD)
      account_internal_addr(account_internal_addr),
      sync_state(sync_state),
#endif  // BUILDFLAG(ENABLE_ORCHARD)
      account_id(account_id.Clone()),
      chain_id(GetNetworkForZCashKeyring(account_id->keyring_id)) {
}

ZCashActionContext& ZCashActionContext::operator=(ZCashActionContext&&) =
    default;
ZCashActionContext::ZCashActionContext(ZCashActionContext&&) = default;

ZCashActionContext::~ZCashActionContext() = default;

}  // namespace brave_wallet
