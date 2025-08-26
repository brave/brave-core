/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_RPC_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_RPC_H_

#include "base/memory/weak_ptr.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_wallet {

class PolkadotRpc {
 public:
  explicit PolkadotRpc(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~PolkadotRpc();

  void GetChainName(base::OnceCallback<void(const std::string&)> callback);

 private:
  using APIRequestResult = api_request_helper::APIRequestResult;

  api_request_helper::APIRequestHelper api_request_helper_;
  base::WeakPtrFactory<PolkadotRpc> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_RPC_H_
