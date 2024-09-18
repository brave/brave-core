/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SIMULATION_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SIMULATION_SERVICE_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "url/gurl.h"

namespace network {

class SharedURLLoaderFactory;

}  // namespace network

namespace brave_wallet {

class BraveWalletService;

class SimulationService : public KeyedService, public mojom::SimulationService {
 public:
  using APIRequestResult = api_request_helper::APIRequestResult;

  SimulationService(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      BraveWalletService* brave_wallet_service);
  ~SimulationService() override;
  SimulationService(const SimulationService&) = delete;
  SimulationService& operator=(const SimulationService&) = delete;

  mojo::PendingRemote<mojom::SimulationService> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::SimulationService> receiver);

  static GURL GetScanMessageURL(const std::string& chain_id,
                                mojom::CoinType coin,
                                const std::string& language);

  static GURL GetScanTransactionURL(const std::string& chain_id,
                                    mojom::CoinType coin,
                                    const std::string& language);

  void HasTransactionScanSupport(
      const std::string& chain_id,
      mojom::CoinType coin,
      HasTransactionScanSupportCallback callback) override;

  void HasMessageScanSupport(const std::string& chain_id,
                             mojom::CoinType coin,
                             HasMessageScanSupportCallback callback) override;

  void ScanEVMTransaction(const std::string& tx_meta_id,
                          const std::string& language,
                          ScanEVMTransactionCallback callback) override;
  void ScanSolanaTransaction(const std::string& tx_meta_id,
                             const std::string& language,
                             ScanSolanaTransactionCallback callback) override;
  void ScanSignSolTransactionsRequest(
      int32_t id,
      const std::string& language,
      ScanSignSolTransactionsRequestCallback callback) override;

 private:
  friend class SimulationServiceUnitTest;

  std::optional<std::string> CanScanTransaction(const std::string& chain_id,
                                                mojom::CoinType coin);

  void ScanEVMTransactionInternal(mojom::TransactionInfoPtr tx_info,
                                  const std::string& language,
                                  ScanEVMTransactionCallback callback);
  void ScanSolanaTransactionInternal(mojom::TransactionInfoPtr tx_info,
                                     const std::string& language,
                                     ScanSolanaTransactionCallback callback);
  void ScanSignSolTransactionsRequestInternal(
      mojom::SignSolTransactionsRequestPtr request,
      const std::string& language,
      ScanSignSolTransactionsRequestCallback callback);

  void OnScanEVMTransaction(ScanEVMTransactionCallback callback,
                            const std::string& user_account,
                            APIRequestResult api_request_result);

  void ContinueScanSolanaTransaction(mojom::TransactionInfoPtr tx_info,
                                     const std::string& language,
                                     ScanSolanaTransactionCallback callback,
                                     const std::string& latest_blockhash,
                                     uint64_t last_valid_block_height,
                                     mojom::SolanaProviderError error,
                                     const std::string& error_message);

  void ContinueScanSignSolTransactionsRequest(
      mojom::SignSolTransactionsRequestPtr request,
      const std::string& language,
      ScanSignSolTransactionsRequestCallback callback,
      const std::string& latest_blockhash,
      uint64_t last_valid_block_height,
      mojom::SolanaProviderError error,
      const std::string& error_message);

  void OnScanSolanaTransaction(ScanSolanaTransactionCallback callback,
                               const std::string& user_account,
                               APIRequestResult api_request_result);

  api_request_helper::APIRequestHelper api_request_helper_;
  raw_ptr<BraveWalletService> brave_wallet_service_ = nullptr;
  mojo::ReceiverSet<mojom::SimulationService> receivers_;
  base::WeakPtrFactory<SimulationService> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SIMULATION_SERVICE_H_
