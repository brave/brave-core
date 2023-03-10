/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_RPC_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_RPC_SERVICE_H_

#include <list>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/receiver_set.h"

class PrefService;

namespace brave_wallet {
struct GetBitcoinAccountInfoContext;
struct SendToContext;

class BitcoinRpcService : public KeyedService, public mojom::BitcoinRpcService {
 public:
  BitcoinRpcService(
      KeyringService* keyring_service,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      PrefService* prefs,
      PrefService* local_state_prefs);
  ~BitcoinRpcService() override;

  mojo::PendingRemote<mojom::BitcoinRpcService> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::BitcoinRpcService> receiver);

  using APIRequestHelper = api_request_helper::APIRequestHelper;
  using APIRequestResult = api_request_helper::APIRequestResult;
  using RequestIntermediateCallback =
      base::OnceCallback<void(APIRequestResult api_request_result)>;

  void GetChainHeight(const std::string& keyring_id,
                      GetChainHeightCallback callback) override;

  void GetUtxoList(const std::string& keyring_id,
                   const std::string& address,
                   GetUtxoListCallback callback) override;

  void GetBitcoinAccountInfo(const std::string& keyring_id,
                             uint32_t account_index,
                             GetBitcoinAccountInfoCallback callback) override;

  void SendTo(const std::string& keyring_id,
              uint32_t account_index,
              const std::string& address_to,
              uint64_t amount,
              uint64_t fee,
              SendToCallback callback) override;

 private:
  void RequestInternal(bool auto_retry_on_network_change,
                       const GURL& request_url,
                       RequestIntermediateCallback callback,
                       APIRequestHelper::ResponseConversionCallback
                           conversion_callback = base::NullCallback());

  //   void ContinueGetBitcoinAccountInfo(
  //       std::unique_ptr<GetBitcoinAccountInfoContext> context);

  //   void GetTransaction(const std::string& keyring_id,
  //                       const std::string& txid,
  //                       base::OnceCallback<void(base::Value)> callback);
  //   void OnGetTransaction(base::OnceCallback<void(base::Value)> callback,
  //                         base::Value transaction);
  void FetchTransaction(const std::string& keyring_id,
                        const std::string& txid,
                        base::OnceCallback<void(base::Value)> callback);
  void OnFetchTransaction(const std::string& txid,
                          base::OnceCallback<void(base::Value)> callback,
                          APIRequestResult api_request_result);

  void OnGetChainHeight(GetChainHeightCallback callback,
                        APIRequestResult api_request_result);

  void OnGetUtxoListForBitcoinAccountInfo(
      scoped_refptr<GetBitcoinAccountInfoContext> context,
      mojom::BitcoinAddressPtr requested_address,
      APIRequestResult api_request_result);

  void OnGetUtxoList(GetUtxoListCallback callback,
                     APIRequestResult api_request_result);

  //  void GetChainHeight(std::unique_ptr<SendToContext> context);
  //   void OnGetChainHeight(std::unique_ptr<SendToContext> context,
  //                         APIRequestResult api_request_result);

  void OnGetChainHeightForSendTo(std::unique_ptr<SendToContext> context,
                                 uint32_t height);
  void OnGetBitcoinAccountInfoForSendTo(
      std::unique_ptr<SendToContext> context,
      mojom::BitcoinAccountInfoPtr account_info);

  bool PickInputs(SendToContext& context);
  bool PrepareOutputs(SendToContext& context);
  void FetchInputTransactions(std::unique_ptr<SendToContext> context);
  void OnFetchTransactionForSendTo(std::unique_ptr<SendToContext> context,
                                   std::string txid,
                                   base::Value transaction);
  bool FillPubkeys(SendToContext& context);
  bool FillSignature(SendToContext& context, uint32_t input_index);
  bool FillSignatures(SendToContext& context);
  bool FillTransaction(SendToContext& context);
  void PostTransaction(std::unique_ptr<SendToContext> context);
  void OnPostTransaction(std::unique_ptr<SendToContext> context,
                         APIRequestResult api_request_result);
  void WorkOnSendTo(std::unique_ptr<SendToContext> context);

  raw_ptr<KeyringService> keyring_service_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  std::unique_ptr<APIRequestHelper> api_request_helper_;
  mojo::ReceiverSet<mojom::BitcoinRpcService> receivers_;
  std::map<std::string, base::Value> transactions_cache_;
  PrefService* prefs_ = nullptr;
  PrefService* local_state_prefs_ = nullptr;
  base::WeakPtrFactory<BitcoinRpcService> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_RPC_SERVICE_H_
