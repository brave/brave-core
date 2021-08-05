/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_EIP1559_TRANSACTION_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_EIP1559_TRANSACTION_H_

#include <memory>
#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/eip2930_transaction.h"

namespace brave_wallet {

class Eip1559Transaction : public Eip2930Transaction {
 public:
  Eip1559Transaction();
  Eip1559Transaction(mojom::TxDataPtr,
                     const std::string& chain_id,
                     const std::string& max_priority_fee_per_gas,
                     const std::string& max_fee_per_gas);

  ~Eip1559Transaction() override;
  bool operator==(const Eip1559Transaction&) const;

  static std::unique_ptr<Eip1559Transaction> FromValue(
      const base::Value& value);

  std::string max_priority_fee_per_gas() const {
    return max_priority_fee_per_gas_;
  }
  std::string max_fee_per_gas() const { return max_fee_per_gas_; }

  // keccak256(0x02 || rlp([chainId, nonce, maxPriorityFeePerGas, maxFeePerGas,
  // gasLimit, destination, value, data, access_list]))
  void GetMessageToSign(const std::string& chain_id,
                        GetMessageToSignCallback) override;

  // 0x02 || rlp([chainId, nonce, maxPriorityFeePerGas, maxFeePerGas, gasLimit,
  // destination, value, data, accessList, signatureYParity, signatureR,
  // signatureS])
  void GetSignedTransaction(GetSignedTransactionCallback) override;

  base::Value ToValue() const override;

  uint256_t GetUpfrontCost(uint256_t block_base_fee = 0) const override;

 protected:
  bool GetBasicListData(base::ListValue* list) const override;

  std::string max_priority_fee_per_gas_;
  std::string max_fee_per_gas_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_EIP1559_TRANSACTION_H_
