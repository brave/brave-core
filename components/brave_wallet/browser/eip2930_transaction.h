/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_EIP2930_TRANSACTION_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_EIP2930_TRANSACTION_H_

#include <array>
#include <optional>
#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/eth_transaction.h"

namespace brave_wallet {

class Eip2930Transaction : public EthTransaction {
 public:
  typedef std::array<uint8_t, 20> AccessedAddress;
  typedef std::array<uint8_t, 32> AccessedStorageKey;
  struct AccessListItem {
    AccessListItem();
    ~AccessListItem();
    AccessListItem(const AccessListItem&);
    bool operator==(const AccessListItem&) const;
    bool operator!=(const AccessListItem&) const;

    AccessedAddress address;
    std::vector<AccessedStorageKey> storage_keys;
  };
  // [[{20 bytes}, [{32 bytes}...]]...]
  typedef std::vector<AccessListItem> AccessList;

  Eip2930Transaction();
  Eip2930Transaction(const Eip2930Transaction&);
  ~Eip2930Transaction() override;
  bool operator==(const Eip2930Transaction&) const;

  static std::optional<Eip2930Transaction> FromTxData(const mojom::TxDataPtr&,
                                                      uint256_t chain_id,
                                                      bool strict = true);
  static std::optional<Eip2930Transaction> FromValue(
      const base::Value::Dict& value);

  static base::Value::List AccessListToValue(const AccessList&);
  static std::optional<AccessList> ValueToAccessList(const base::Value::List&);

  uint256_t chain_id() const { return chain_id_; }
  const AccessList* access_list() const { return &access_list_; }
  AccessList* access_list() { return &access_list_; }

  // keccak256(0x01 || rlp([chainId, nonce, gasPrice, gasLimit, to, value, data,
  // accessList]))
  std::vector<uint8_t> GetMessageToSign(uint256_t chain_id = 0,
                                        bool hash = true) const override;

  // 0x01 || rlp([chainId, nonce, gasPrice, gasLimit, to, value, data,
  // accessList, signatureYParity, signatureR, signatureS])
  std::string GetSignedTransaction() const override;

  // keccack(0x01 || rlp([chainId, nonce, gasPrice, gasLimit, to, value, data,
  // accessList, signatureYParity, signatureR, signatureS]))
  std::string GetTransactionHash() const override;

  void ProcessSignature(const std::vector<uint8_t> signature,
                        int recid,
                        uint256_t chain_id = 0) override;

  bool IsSigned() const override;

  base::Value::Dict ToValue() const override;

  uint256_t GetDataFee() const override;

 protected:
  Eip2930Transaction(std::optional<uint256_t> nonce,
                     uint256_t gas_price,
                     uint256_t gas_limit,
                     const EthAddress& to,
                     uint256_t value,
                     const std::vector<uint8_t>& data,
                     uint256_t chain_id);

  uint256_t chain_id_;
  AccessList access_list_;

 private:
  std::vector<uint8_t> Serialize() const;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_EIP2930_TRANSACTION_H_
