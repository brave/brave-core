/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_TRANSACTION_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_TRANSACTION_H_

#include <optional>
#include <string>
#include <vector>

#include "base/gtest_prod_util.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "brave/components/brave_wallet/common/hash_utils.h"

namespace base {
class Value;
}  // namespace base

namespace brave_wallet {
FORWARD_DECLARE_TEST(EthTransactionTest, GetSignedTransaction);
FORWARD_DECLARE_TEST(EthTransactionTest, TransactionAndValue);
FORWARD_DECLARE_TEST(Eip2930TransactionUnitTest, GetSignedTransaction);

// TODO(apaymyshev): make use of that enum instead of magic numbers.
// https://eips.ethereum.org/EIPS/eip-2718
enum EthTransactionType : uint8_t {
  kLegacy = 0,
  kEip2930 = 1,  // https://eips.ethereum.org/EIPS/eip-2930#definitions
  kEip1559 = 2   // https://eips.ethereum.org/EIPS/eip-1559#specification
};

class EthTransaction {
 public:
  EthTransaction();
  EthTransaction(const EthTransaction&);
  virtual ~EthTransaction();
  bool operator==(const EthTransaction&) const;

  static std::optional<EthTransaction> FromTxData(
      const mojom::TxDataPtr& tx_data,
      bool strict = true);
  static std::optional<EthTransaction> FromValue(
      const base::Value::Dict& value);

  uint8_t type() const { return type_; }

  std::optional<uint256_t> nonce() const { return nonce_; }
  uint256_t gas_price() const { return gas_price_; }
  uint256_t gas_limit() const { return gas_limit_; }
  EthAddress to() const { return to_; }
  uint256_t value() const { return value_; }
  std::vector<uint8_t> data() const { return data_; }
  uint256_t v() const { return v_; }
  std::vector<uint8_t> r() const { return r_; }
  std::vector<uint8_t> s() const { return s_; }

  void set_to(EthAddress to) { to_ = to; }
  void set_value(uint256_t value) { value_ = value; }
  void set_nonce(std::optional<uint256_t> nonce) { nonce_ = nonce; }
  void set_data(const std::vector<uint8_t>& data) { data_ = data; }
  void set_gas_price(uint256_t gas_price) { gas_price_ = gas_price; }
  void set_gas_limit(uint256_t gas_limit) { gas_limit_ = gas_limit; }
  bool ProcessVRS(const std::vector<uint8_t>& v,
                  const std::vector<uint8_t>& r,
                  const std::vector<uint8_t>& s);
  bool IsToCreationAddress() const { return to_.IsEmpty(); }

  // return rlp([nonce, gasPrice, gasLimit, to, value, data, chainID, 0, 0])
  // Support EIP-155 chain id
  virtual std::vector<uint8_t> GetMessageToSign(uint256_t chain_id) const;

  // keccak(GetMessageToSign(chain_id))
  KeccakHashArray GetHashedMessageToSign(uint256_t chain_id) const;

  // return rlp([nonce, gasPrice, gasLimit, to, value, data, v, r, s])
  virtual std::string GetSignedTransaction() const;

  // return keccack(rlp([nonce, gasPrice, gasLimit, to, value, data, v, r, s]))
  virtual std::string GetTransactionHash() const;

  // signature and recid will be used to produce v, r, s
  // Support EIP-155 chain id
  virtual void ProcessSignature(base::span<const uint8_t> signature,
                                int recid,
                                uint256_t chain_id);

  virtual bool IsSigned() const;

  virtual base::Value::Dict ToValue() const;

  // Minimum gas required (data fee + tx fee + contract creation fee)
  uint256_t GetBaseFee() const;
  // Gas paid for the data.
  virtual uint256_t GetDataFee() const;
  // The up front amount that an account must have for this transaction to be
  // valid
  virtual uint256_t GetUpfrontCost(uint256_t block_base_fee = 0) const;

 protected:
  // type 0 would be LegacyTransaction
  uint8_t type_ = 0;

  std::optional<uint256_t> nonce_;
  uint256_t gas_price_;
  uint256_t gas_limit_;
  EthAddress to_;
  uint256_t value_;
  std::vector<uint8_t> data_;

  uint256_t v_ = 0;
  std::vector<uint8_t> r_;
  std::vector<uint8_t> s_;

 protected:
  EthTransaction(std::optional<uint256_t> nonce,
                 uint256_t gas_price,
                 uint256_t gas_limit,
                 const EthAddress& to,
                 uint256_t value,
                 const std::vector<uint8_t>& data);

 private:
  FRIEND_TEST_ALL_PREFIXES(EthTransactionUnitTest, GetSignedTransactionAndHash);
  FRIEND_TEST_ALL_PREFIXES(EthTransactionUnitTest, TransactionAndValue);
  FRIEND_TEST_ALL_PREFIXES(Eip2930TransactionUnitTest,
                           GetSignedTransactionAndHash);

  base::Value Serialize() const;
};

}  // namespace brave_wallet
#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_TRANSACTION_H_
