/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_RPC_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_RPC_H_

#include <list>
#include <memory>
#include <string>

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_grpc_utils.h"
#include "brave/components/services/brave_wallet/public/mojom/zcash_decoder.mojom.h"
#include "components/prefs/pref_service.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/cpp/simple_url_loader_stream_consumer.h"

namespace brave_wallet {

// lightwalletd interface
class ZCashRpc {
 public:
  using GetUtxoListCallback = base::OnceCallback<void(
      base::expected<mojom::GetAddressUtxosResponsePtr, std::string>)>;
  using GetLatestBlockCallback =
      base::OnceCallback<void(base::expected<mojom::BlockIDPtr, std::string>)>;
  using GetTransactionCallback = base::OnceCallback<void(
      base::expected<mojom::RawTransactionPtr, std::string>)>;
  using SendTransactionCallback = base::OnceCallback<void(
      base::expected<mojom::SendResponsePtr, std::string>)>;
  using GetTransactionsCallback = base::OnceCallback<void(
      base::expected<mojom::SendResponsePtr, std::string>)>;
  using IsKnownAddressCallback =
      base::OnceCallback<void(base::expected<bool, std::string>)>;

  ZCashRpc(NetworkManager* network_manager,
           scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  virtual ~ZCashRpc();

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

  virtual void IsKnownAddress(const std::string& chain_id,
                              const std::string& addr,
                              uint64_t block_start,
                              uint64_t block_end,
                              IsKnownAddressCallback callback);

 private:
  friend class base::RefCountedThreadSafe<ZCashRpc>;

  using UrlLoadersList = std::list<std::unique_ptr<network::SimpleURLLoader>>;
  using StreamHandlersList =
      std::list<std::unique_ptr<GRrpcMessageStreamHandler>>;

  void OnGetUtxosResponse(ZCashRpc::GetUtxoListCallback callback,
                          UrlLoadersList::iterator it,
                          std::unique_ptr<std::string> response_body);

  void OnGetLatestBlockResponse(ZCashRpc::GetLatestBlockCallback callback,
                                UrlLoadersList::iterator it,
                                std::unique_ptr<std::string> response_body);

  void OnGetTransactionResponse(ZCashRpc::GetTransactionCallback callback,
                                UrlLoadersList::iterator it,
                                std::unique_ptr<std::string> response_body);

  void OnSendTransactionResponse(ZCashRpc::SendTransactionCallback callback,
                                 UrlLoadersList::iterator it,
                                 std::unique_ptr<std::string> response_body);

  void OnGetAddressTxResponse(ZCashRpc::IsKnownAddressCallback callback,
                              UrlLoadersList::iterator it,
                              StreamHandlersList::iterator handler_it,
                              base::expected<bool, std::string> result);

  template <typename T>
  void OnParseResult(base::OnceCallback<void(base::expected<T, std::string>)>,
                     T value);

  mojo::AssociatedRemote<mojom::ZCashDecoder>& GetDecoder();

  GURL GetNetworkURL(const std::string& chain_id);

  UrlLoadersList url_loaders_list_;
  StreamHandlersList stream_handlers_list_;
  raw_ptr<NetworkManager> network_manager_ = nullptr;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_ = nullptr;

  mojo::AssociatedRemote<mojom::ZCashDecoder> zcash_decoder_;

  base::WeakPtrFactory<ZCashRpc> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_RPC_H_
