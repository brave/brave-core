/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_TX_STATE_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_TX_STATE_MANAGER_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/time/time.h"
#include "brave/components/brave_wallet/browser/brave_wallet_types.h"
#include "brave/components/brave_wallet/browser/eth_address.h"
#include "brave/components/brave_wallet/browser/eth_json_rpc_controller.h"
#include "brave/components/brave_wallet/browser/eth_transaction.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

class PrefService;

namespace base {
class Value;
}  // namespace base

namespace brave_wallet {

class EthJsonRpcController;

class EthTxStateManager : public mojom::EthJsonRpcControllerObserver {
 public:
  struct TxMeta {
    TxMeta();
    explicit TxMeta(std::unique_ptr<EthTransaction> tx);
    TxMeta(const TxMeta&) = delete;
    ~TxMeta();
    bool operator==(const TxMeta&) const;

    std::string id;
    mojom::TransactionStatus status = mojom::TransactionStatus::Unapproved;
    EthAddress from;
    uint256_t last_gas_price = 0;
    base::Time created_time;
    base::Time submitted_time;
    base::Time confirmed_time;
    TransactionReceipt tx_receipt;
    std::string tx_hash;
    std::unique_ptr<EthTransaction> tx;
  };

  explicit EthTxStateManager(
      PrefService* prefs,
      mojo::PendingRemote<mojom::EthJsonRpcController> rpc_controller);
  ~EthTxStateManager() override;
  EthTxStateManager(const EthTxStateManager&) = delete;
  EthTxStateManager operator=(const EthTxStateManager&) = delete;

  static std::string GenerateMetaID();
  static base::Value TxMetaToValue(const TxMeta& meta);
  static mojom::TransactionInfoPtr TxMetaToTransactionInfo(const TxMeta& meta);
  static std::unique_ptr<TxMeta> ValueToTxMeta(const base::Value& value);

  void AddOrUpdateTx(const TxMeta& meta);
  std::unique_ptr<TxMeta> GetTx(const std::string& id);
  void DeleteTx(const std::string& id);
  void WipeTxs();

  std::vector<std::unique_ptr<TxMeta>> GetTransactionsByStatus(
      absl::optional<mojom::TransactionStatus> status,
      absl::optional<EthAddress> from);

  // mojom::EthJsonRpcControllerObserver
  void ChainChangedEvent(const std::string& chain_id) override;
  void OnAddEthereumChainRequestCompleted(const std::string& chain_id,
                                          const std::string& error) override;

  void SetChainCallbackForTesting(base::OnceClosure callback) {
    chain_callback_for_testing_ = std::move(callback);
  }

 private:
  std::string GetNetworkId() const;
  // only support REJECTED and CONFIRMED
  void RetireTxByStatus(mojom::TransactionStatus status, size_t max_num);

  void OnConnectionError();
  void OnGetNetworkUrl(const std::string& url);
  void OnGetChainId(const std::string& chain_id);

  PrefService* prefs_;
  mojo::Remote<mojom::EthJsonRpcController> rpc_controller_;
  mojo::Receiver<mojom::EthJsonRpcControllerObserver> observer_receiver_{this};
  std::string chain_id_;
  std::string network_url_;
  base::OnceClosure chain_callback_for_testing_;
  base::WeakPtrFactory<EthTxStateManager> weak_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_TX_STATE_MANAGER_H_
