/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_EIP1559_TRANSACTION_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_EIP1559_TRANSACTION_H_

#include <optional>
#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/eip2930_transaction.h"

namespace brave_wallet {

class Eip1559Transaction : public Eip2930Transaction {
 public:
  struct GasEstimation {
    GasEstimation() = default;
    ~GasEstimation() = default;
    GasEstimation(const GasEstimation&) = default;
    bool operator==(const GasEstimation&) const;

    static std::optional<GasEstimation> FromMojomGasEstimation1559(
        mojom::GasEstimation1559Ptr gas_estimation);
    static mojom::GasEstimation1559Ptr ToMojomGasEstimation1559(
        GasEstimation gas_estimation);

    uint256_t slow_max_priority_fee_per_gas = 0;
    uint256_t avg_max_priority_fee_per_gas = 0;
    uint256_t fast_max_priority_fee_per_gas = 0;
    uint256_t slow_max_fee_per_gas = 0;
    uint256_t avg_max_fee_per_gas = 0;
    uint256_t fast_max_fee_per_gas = 0;
    uint256_t base_fee_per_gas = 0;
  };

  Eip1559Transaction();
  Eip1559Transaction(const Eip1559Transaction&);
  ~Eip1559Transaction() override;
  bool operator==(const Eip1559Transaction&) const;

  static std::optional<Eip1559Transaction> FromTxData(
      const mojom::TxData1559Ptr& tx_data,
      bool strict = true);
  static std::optional<Eip1559Transaction> FromValue(
      const base::Value::Dict& value);

  uint256_t max_priority_fee_per_gas() const {
    return max_priority_fee_per_gas_;
  }
  uint256_t max_fee_per_gas() const { return max_fee_per_gas_; }
  GasEstimation gas_estimation() const { return gas_estimation_; }

  void set_max_fee_per_gas(uint256_t max_fee_per_gas) {
    max_fee_per_gas_ = max_fee_per_gas;
  }
  void set_max_priority_fee_per_gas(uint256_t max_priority_fee_per_gas) {
    max_priority_fee_per_gas_ = max_priority_fee_per_gas;
  }
  void set_gas_estimation(GasEstimation estimation) {
    gas_estimation_ = estimation;
  }

  // 0x02 || rlp([chainId, nonce, maxPriorityFeePerGas, maxFeePerGas,
  // gasLimit, destination, value, data, access_list])
  std::vector<uint8_t> GetMessageToSign(uint256_t chain_id) const override;

  // 0x02 || rlp([chainId, nonce, maxPriorityFeePerGas, maxFeePerGas, gasLimit,
  // destination, value, data, accessList, signatureYParity, signatureR,
  // signatureS])
  std::string GetSignedTransaction() const override;

  // keccacak(0x02 || rlp([chainId, nonce, maxPriorityFeePerGas, maxFeePerGas,
  // gasLimit, destination, value, data, accessList, signatureYParity,
  // signatureR,signatureS]))
  std::string GetTransactionHash() const override;

  base::Value::Dict ToValue() const override;

  uint256_t GetUpfrontCost(uint256_t block_base_fee = 0) const override;

 protected:
  Eip1559Transaction(std::optional<uint256_t> nonce,
                     uint256_t gas_price,
                     uint256_t gas_limit,
                     const EthAddress& to,
                     uint256_t value,
                     const std::vector<uint8_t>& data,
                     uint256_t chain_id,
                     uint256_t max_priority_fee_per_gas,
                     uint256_t max_fee_per_gas,
                     GasEstimation gas_estimation);

  uint256_t max_priority_fee_per_gas_;
  uint256_t max_fee_per_gas_;

  // Gas estimation result
  GasEstimation gas_estimation_;

 private:
  std::vector<uint8_t> Serialize() const;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_EIP1559_TRANSACTION_H_
