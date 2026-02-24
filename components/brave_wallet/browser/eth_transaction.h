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
#include "brave/components/brave_wallet/browser/internal/secp256k1_signature.h"
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

// https://eips.ethereum.org/EIPS/eip-2718
enum class EthTransactionType : uint8_t {
  kLegacy = 0,
  kEip2930 = 1,  // https://eips.ethereum.org/EIPS/eip-2930#definitions
  kEip1559 = 2   // https://eips.ethereum.org/EIPS/eip-1559#specification
};

class EthTransaction {
 public:
  EthTransaction();
  EthTransaction(const EthTransaction&);
  virtual ~EthTransaction();
  bool operator==(const EthTransaction&) const = default;

  static std::optional<EthTransaction> FromTxData(
      const mojom::TxDataPtr& tx_data,
      bool strict = true);
  static std::optional<EthTransaction> FromValue(const base::DictValue& value);

  EthTransactionType type() const { return type_; }

  std::optional<uint256_t> nonce() const { return nonce_; }
  uint256_t gas_price() const { return gas_price_; }
  uint256_t gas_limit() const { return gas_limit_; }
  std::variant<EthAddress, EthContractCreationAddress> to() const {
    return to_;
  }
  uint256_t value() const { return value_; }
  std::vector<uint8_t> data() const { return data_; }
  uint256_t v() const { return v_; }
  std::vector<uint8_t> r() const { return r_; }
  std::vector<uint8_t> s() const { return s_; }

  void set_to(const EthAddress& to) { to_ = to; }
  void set_to(const EthContractCreationAddress& to) { to_ = to; }
  void set_value(uint256_t value) { value_ = value; }
  void set_nonce(std::optional<uint256_t> nonce) { nonce_ = nonce; }
  void set_data(const std::vector<uint8_t>& data) { data_ = data; }
  void set_gas_price(uint256_t gas_price) { gas_price_ = gas_price; }
  void set_gas_limit(uint256_t gas_limit) { gas_limit_ = gas_limit; }
  bool ProcessVRS(const std::vector<uint8_t>& v,
                  const std::vector<uint8_t>& r,
                  const std::vector<uint8_t>& s);
  bool IsToCreationAddress() const {
    return std::get_if<EthContractCreationAddress>(&to_);
  }

  // Creates binary message for signing depending on transaction's version.
  std::vector<uint8_t> GetMessageToSign(uint256_t chain_id) const;

  // keccak(GetMessageToSign(chain_id))
  KeccakHashArray GetHashedMessageToSign(uint256_t chain_id) const;

  // Returns hex of serialized transaction.
  std::string GetSignedTransaction() const;

  // Returns hex of serialized transaction keccak hash.
  std::string GetTransactionHash() const;

  // signature and recid will be used to produce v, r, s
  // Support EIP-155 chain id
  void ProcessSignature(const Secp256k1Signature& signature,
                        uint256_t chain_id);

  bool IsSigned() const;

  base::DictValue ToValue() const;

  // Minimum gas required (data fee + tx fee + contract creation fee)
  uint256_t GetBaseFee() const;

  std::string GetToHex() const;
  std::string GetToChecksumAddress() const;

 protected:
  FRIEND_TEST_ALL_PREFIXES(EthTransactionUnitTest, GetSignedTransactionAndHash);
  FRIEND_TEST_ALL_PREFIXES(EthTransactionUnitTest, TransactionAndValue);
  FRIEND_TEST_ALL_PREFIXES(EthTransactionUnitTest, GetDataFee);
  FRIEND_TEST_ALL_PREFIXES(Eip2930TransactionUnitTest,
                           GetSignedTransactionAndHash);

  EthTransaction(std::optional<uint256_t> nonce,
                 uint256_t gas_price,
                 uint256_t gas_limit,
                 std::variant<EthAddress, EthContractCreationAddress> to,
                 uint256_t value,
                 const std::vector<uint8_t>& data);

  std::vector<uint8_t> GetToBytes() const;

  // return rlp([nonce, gasPrice, gasLimit, to, value, data, chainID, 0, 0])
  // Support EIP-155 chain id
  virtual std::vector<uint8_t> GetMessageToSignImpl(uint256_t chain_id) const;

  // Gas paid for the data.
  virtual uint256_t GetDataFee() const;

  virtual base::DictValue ToValueImpl() const;

  // keccak(rlp([nonce, gasPrice, gasLimit, to, value, data, v, r, s]))
  virtual std::vector<uint8_t> Serialize() const;

  EthTransactionType type_ = EthTransactionType::kLegacy;

  std::optional<uint256_t> nonce_;
  uint256_t gas_price_;
  uint256_t gas_limit_;
  std::variant<EthAddress, EthContractCreationAddress> to_;
  uint256_t value_;
  std::vector<uint8_t> data_;

  uint256_t v_ = 0;
  std::vector<uint8_t> r_;
  std::vector<uint8_t> s_;
};

}  // namespace brave_wallet
#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_TRANSACTION_H_
