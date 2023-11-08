/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_RPC_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_RPC_H_

#include <list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/zcash/protos/zcash_grpc_data.pb.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_service.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_wallet {

// lightwalletd interface
class ZCashRpc : public KeyedService {
 public:
  using GetUtxoListCallback = base::OnceCallback<void(
      base::expected<std::vector<zcash::ZCashUtxo>, std::string>)>;
  using GetLatestBlockCallback =
      base::OnceCallback<void(base::expected<zcash::BlockID, std::string>)>;
  using GetTransactionCallback = base::OnceCallback<void(
      base::expected<zcash::RawTransaction, std::string>)>;
  using SendTransactionCallback = base::OnceCallback<void(
      base::expected<zcash::SendResponse, std::string>)>;

  ZCashRpc(PrefService* prefs,
           scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~ZCashRpc() override;

  virtual void GetUtxoList(const std::string& chain_id,
                           const std::string& address,
                           GetUtxoListCallback callback);

  virtual void GetLatestBlock(const std::string& chain_id,
                              GetLatestBlockCallback callback);

  virtual void GetTransaction(const std::string& chain_id,
                              const std::string& tx_hash,
                              GetTransactionCallback callback);

  virtual void SendTransaction(const std::string& chain_id,
                               const std::string& data,
                               SendTransactionCallback callback);

 private:
  friend class base::RefCountedThreadSafe<ZCashRpc>;

  using UrlLoadersList = std::list<std::unique_ptr<network::SimpleURLLoader>>;

  void OnGetUtxosResponse(ZCashRpc::GetUtxoListCallback callback,
                          UrlLoadersList::iterator it,
                          const std::unique_ptr<std::string> response_body);

  void OnGetLatestBlockResponse(
      ZCashRpc::GetLatestBlockCallback callback,
      UrlLoadersList::iterator it,
      const std::unique_ptr<std::string> response_body);

  void OnGetTransactionResponse(
      ZCashRpc::GetTransactionCallback callback,
      UrlLoadersList::iterator it,
      const std::unique_ptr<std::string> response_body);

  void OnSendTransactionResponse(
      ZCashRpc::SendTransactionCallback callback,
      UrlLoadersList::iterator it,
      const std::unique_ptr<std::string> response_body);

  UrlLoadersList url_loaders_list_;
  raw_ptr<PrefService> prefs_ = nullptr;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_ = nullptr;
  base::WeakPtrFactory<ZCashRpc> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_RPC_H_
