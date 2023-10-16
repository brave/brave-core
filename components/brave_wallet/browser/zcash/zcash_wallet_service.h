/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_WALLET_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_WALLET_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/keyring_service_observer_base.h"
#include "brave/components/brave_wallet/browser/zcash/protos/zcash_grpc_data.pb.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_rpc.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/keyed_service/core/keyed_service.h"

namespace brave_wallet {

class GetTransparentBalanceContext;

class ZCashWalletService : public KeyedService,
                           public mojom::ZCashWalletService,
                           KeyringServiceObserverBase {
 public:
  ZCashWalletService(
      KeyringService* keyring_service,
      PrefService* prefs,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~ZCashWalletService() override;

  mojo::PendingRemote<mojom::ZCashWalletService> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::ZCashWalletService> receiver);

  void GetBalance(const std::string& chain_id,
                  mojom::AccountIdPtr account_id,
                  GetBalanceCallback) override;

 private:
  void OnGetUtxosForBalance(
      scoped_refptr<GetTransparentBalanceContext> context,
      const std::string& current_address,
      base::expected<std::vector<zcash::ZCashUtxo>, std::string> result);
  void WorkOnGetBalance(scoped_refptr<GetTransparentBalanceContext> context);

  raw_ptr<KeyringService> keyring_service_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  mojo::ReceiverSet<mojom::ZCashWalletService> receivers_;
  std::unique_ptr<zcash_rpc::ZCashRpc> zcash_rpc_;
  base::WeakPtrFactory<ZCashWalletService> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_WALLET_SERVICE_H_
