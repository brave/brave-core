/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_EIP2930_TRANSACTION_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_EIP2930_TRANSACTION_H_

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/eth_transaction.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

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
  Eip2930Transaction(mojom::TxDataPtr, const std::string& chain_id);

  ~Eip2930Transaction() override;
  bool operator==(const Eip2930Transaction&) const;

  static std::unique_ptr<Eip2930Transaction> FromValue(
      const base::Value& value);

  static std::vector<base::Value> AccessListToValue(const AccessList&);
  static absl::optional<AccessList> ValueToAccessList(const base::Value&);

  std::string chain_id() const { return chain_id_; }
  const AccessList* access_list() const { return &access_list_; }
  AccessList* access_list() { return &access_list_; }

  // keccak256(0x01 || rlp([chainId, nonce, gasPrice, gasLimit, to, value, data,
  // accessList]))
  void GetMessageToSign(const std::string& chain_id,
                        GetMessageToSignCallback) override;

  // 0x01 || rlp([chainId, nonce, gasPrice, gasLimit, to, value, data,
  // accessList, signatureYParity, signatureR, signatureS])
  void GetSignedTransaction(GetSignedTransactionCallback) override;

  void ProcessSignature(const std::vector<uint8_t>& signature,
                        int recid,
                        const std::string& chain_id) override;

  bool IsSigned() const override;

  base::Value ToValue() const override;

  uint256_t GetDataFee() const override;

 protected:
  bool GetBasicListData(base::ListValue* list) const override;

  std::string chain_id_;
  AccessList access_list_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_EIP2930_TRANSACTION_H_
