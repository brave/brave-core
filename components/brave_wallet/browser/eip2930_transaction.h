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
  using AccessedAddress = std::array<uint8_t, 20>;
  using AccessedStorageKey = std::array<uint8_t, 32>;
  struct AccessListItem {
    AccessListItem();
    ~AccessListItem();
    AccessListItem(const AccessListItem&);
    bool operator==(const AccessListItem&) const = default;

    AccessedAddress address;
    std::vector<AccessedStorageKey> storage_keys;
  };
  // [[{20 bytes}, [{32 bytes}...]]...]
  using AccessList = std::vector<AccessListItem>;

  Eip2930Transaction();
  Eip2930Transaction(const Eip2930Transaction&);
  ~Eip2930Transaction() override;
  bool operator==(const Eip2930Transaction&) const = default;

  static std::optional<Eip2930Transaction> FromTxData(const mojom::TxDataPtr&,
                                                      bool strict = true);
  static std::optional<Eip2930Transaction> FromValue(
      const base::DictValue& value);

  static base::ListValue AccessListToValue(const AccessList&);
  static std::optional<AccessList> ValueToAccessList(const base::ListValue&);

  const AccessList* access_list() const { return &access_list_; }
  AccessList* access_list() { return &access_list_; }

 protected:
  Eip2930Transaction(uint256_t chain_id,
                     std::optional<uint256_t> nonce,
                     uint256_t gas_price,
                     uint256_t gas_limit,
                     std::variant<EthAddress, EthContractCreationAddress> to,
                     uint256_t value,
                     const std::vector<uint8_t>& data);

  uint256_t GetDataFee() const override;

  // 0x01 || rlp([chainId, nonce, gasPrice, gasLimit, to, value, data,
  // accessList])
  std::vector<uint8_t> GetMessageToSignImpl() const override;

  // 0x01 || rlp([chainId, nonce, gasPrice, gasLimit, to, value, data,
  // accessList, signatureYParity, signatureR, signatureS])
  std::vector<uint8_t> Serialize() const override;

  base::DictValue ToValueImpl() const override;

  AccessList access_list_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_EIP2930_TRANSACTION_H_
