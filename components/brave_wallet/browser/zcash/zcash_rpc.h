/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_RPC_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_RPC_H_

#include <list>
#include <memory>
#include <string>
#include <vector>

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
      base::expected<zcash::mojom::GetAddressUtxosResponsePtr, std::string>)>;
  using GetLatestBlockCallback = base::OnceCallback<void(
      base::expected<zcash::mojom::BlockIDPtr, std::string>)>;
  using GetTransactionCallback = base::OnceCallback<void(
      base::expected<zcash::mojom::RawTransactionPtr, std::string>)>;
  using SendTransactionCallback = base::OnceCallback<void(
      base::expected<zcash::mojom::SendResponsePtr, std::string>)>;
  using GetTransactionsCallback = base::OnceCallback<void(
      base::expected<zcash::mojom::SendResponsePtr, std::string>)>;
  using IsKnownAddressCallback =
      base::OnceCallback<void(base::expected<bool, std::string>)>;
  using GetTreeStateCallback = base::OnceCallback<void(
      base::expected<zcash::mojom::TreeStatePtr, std::string>)>;
  using GetCompactBlocksCallback = base::OnceCallback<void(
      base::expected<std::vector<zcash::mojom::CompactBlockPtr>, std::string>)>;
  using GetLightdInfoCallback = base::OnceCallback<void(
      base::expected<zcash::mojom::LightdInfoPtr, std::string>)>;

  ZCashRpc(NetworkManager* network_manager,
           scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  virtual ~ZCashRpc();

  virtual void GetTreeState(const std::string& chain_id,
                            zcash::mojom::BlockIDPtr block_id,
                            GetTreeStateCallback callback);
  virtual void GetLatestTreeState(const std::string& chain_id,
                                  GetTreeStateCallback callback);

  virtual void GetUtxoList(const std::string& chain_id,
                           const std::string& address,
                           GetUtxoListCallback callback);

  virtual void GetLatestBlock(const std::string& chain_id,
                              GetLatestBlockCallback callback);

  virtual void GetTransaction(const std::string& chain_id,
                              const std::string& tx_hash,
                              GetTransactionCallback callback);

  virtual void SendTransaction(const std::string& chain_id,
                               base::span<const uint8_t> data,
                               SendTransactionCallback callback);

  virtual void IsKnownAddress(const std::string& chain_id,
                              const std::string& addr,
                              uint64_t block_start,
                              uint64_t block_end,
                              IsKnownAddressCallback callback);

  virtual void GetCompactBlocks(const std::string& chain_id,
                                uint32_t from,
                                uint32_t to,
                                GetCompactBlocksCallback callback);

  virtual void GetLightdInfo(const std::string& chain_id,
                             GetLightdInfoCallback callback);

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

  void OnGetTreeStateResponse(ZCashRpc::GetTreeStateCallback callback,
                              UrlLoadersList::iterator it,
                              std::unique_ptr<std::string> response_body);

  void OnGetCompactBlocksResponse(
      ZCashRpc::GetCompactBlocksCallback callback,
      UrlLoadersList::iterator it,
      StreamHandlersList::iterator handler_it,
      base::expected<std::vector<std::string>, std::string> result);

  void OnGetLightdInfoResponse(GetLightdInfoCallback callback,
                               UrlLoadersList::iterator it,
                               std::unique_ptr<std::string> response_body);

  template <typename T>
  void OnParseResult(base::OnceCallback<void(base::expected<T, std::string>)>,
                     T value);

  void OnParseCompactBlocks(
      GetCompactBlocksCallback callback,
      std::optional<std::vector<zcash::mojom::CompactBlockPtr>> compact_blocks);

  mojo::AssociatedRemote<zcash::mojom::ZCashDecoder>& GetDecoder();

  GURL GetNetworkURL(const std::string& chain_id);

  UrlLoadersList url_loaders_list_;
  StreamHandlersList stream_handlers_list_;
  raw_ptr<NetworkManager> network_manager_ = nullptr;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_ = nullptr;

  mojo::AssociatedRemote<zcash::mojom::ZCashDecoder> zcash_decoder_;

  base::WeakPtrFactory<ZCashRpc> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_RPC_H_
