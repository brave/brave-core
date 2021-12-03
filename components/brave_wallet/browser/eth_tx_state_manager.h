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

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/time/time.h"
#include "brave/components/brave_wallet/browser/eth_transaction.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

class PrefService;

namespace base {
class Value;
}  // namespace base

namespace brave_wallet {

class JsonRpcService;

class EthTxStateManager : public mojom::JsonRpcServiceObserver {
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
    base::Time created_time;
    base::Time submitted_time;
    base::Time confirmed_time;
    TransactionReceipt tx_receipt;
    std::string tx_hash;
    std::unique_ptr<EthTransaction> tx;
  };

  explicit EthTxStateManager(PrefService* prefs,
                             JsonRpcService* json_rpc_service);
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

  // mojom::JsonRpcServiceObserver
  void ChainChangedEvent(const std::string& chain_id) override;
  void OnAddEthereumChainRequestCompleted(const std::string& chain_id,
                                          const std::string& error) override;
  void OnIsEip1559Changed(const std::string& chain_id,
                          bool is_eip1559) override {}

  class Observer : public base::CheckedObserver {
   public:
    virtual void OnTransactionStatusChanged(mojom::TransactionInfoPtr tx_info) {
    }
    virtual void OnNewUnapprovedTx(mojom::TransactionInfoPtr tx_info) {}
  };
  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

 private:
  // only support REJECTED and CONFIRMED
  void RetireTxByStatus(mojom::TransactionStatus status, size_t max_num);

  base::ObserverList<Observer> observers_;
  raw_ptr<PrefService> prefs_ = nullptr;
  raw_ptr<JsonRpcService> json_rpc_service_ = nullptr;
  mojo::Receiver<mojom::JsonRpcServiceObserver> observer_receiver_{this};
  std::string chain_id_;
  std::string network_url_;
  base::WeakPtrFactory<EthTxStateManager> weak_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_TX_STATE_MANAGER_H_
