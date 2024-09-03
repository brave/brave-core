/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_TRANSACTION_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_TRANSACTION_H_

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/gtest_prod_util.h"
#include "brave/components/brave_wallet/browser/solana_message.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace base {
class Value;
}  // namespace base

namespace brave_wallet {

class KeyringService;
class SolanaInstruction;

class SolanaTransaction {
 public:
  struct SendOptions {
    SendOptions();
    ~SendOptions();
    SendOptions(const SendOptions&);
    SendOptions(std::optional<uint64_t> max_retries_param,
                std::optional<std::string> preflight_commitment_param,
                std::optional<bool> skip_preflight_param);

    bool operator==(const SolanaTransaction::SendOptions&) const;
    bool operator!=(const SolanaTransaction::SendOptions&) const;

    static std::optional<SendOptions> FromValue(const base::Value::Dict& value);
    static std::optional<SendOptions> FromValue(
        std::optional<base::Value::Dict> value);
    base::Value::Dict ToValue() const;
    static std::optional<SendOptions> FromMojomSendOptions(
        mojom::SolanaSendTransactionOptionsPtr mojom_options);
    mojom::SolanaSendTransactionOptionsPtr ToMojomSendOptions() const;

    std::optional<uint64_t> max_retries;
    std::optional<std::string> preflight_commitment;
    std::optional<bool> skip_preflight;
  };

  explicit SolanaTransaction(SolanaMessage message);

  SolanaTransaction(
      mojom::SolanaMessageVersion version,
      const std::string& recent_blockhash,
      uint64_t last_valid_block_height,
      const std::string& fee_payer,
      const SolanaMessageHeader& message_header,
      std::vector<SolanaAddress> static_account_keys,
      std::vector<SolanaInstruction> instructions,
      std::vector<SolanaMessageAddressTableLookup> addr_table_lookups);

  SolanaTransaction(SolanaMessage message, std::vector<uint8_t> raw_signatures);
  SolanaTransaction(SolanaMessage message,
                    mojom::SolanaSignTransactionParamPtr sign_tx_param);
  SolanaTransaction(const SolanaTransaction&) = delete;
  SolanaTransaction& operator=(const SolanaTransaction&) = delete;
  ~SolanaTransaction();
  bool operator==(const SolanaTransaction&) const;
  bool operator!=(const SolanaTransaction&) const;

  // Serialize the message and sign it with the selected account.
  std::string GetSignedTransaction(
      KeyringService* keyring_service,
      const mojom::AccountIdPtr& selected_account) const;
  std::optional<std::vector<uint8_t>> GetSignedTransactionBytes(
      KeyringService* keyring_service,
      const mojom::AccountIdPtr& selected_account,
      const std::vector<uint8_t>* selected_account_signature = nullptr) const;

  // https://docs.rs/solana-sdk/1.18.14/src/solana_sdk/transaction/mod.rs.html#271-276
  std::string GetUnsignedTransaction() const;

  // Serialize and encode the message in Base64.
  std::string GetBase64EncodedMessage() const;

  // Returns message bytes and signer addresses (public keys).
  std::optional<std::pair<std::vector<uint8_t>, std::vector<std::string>>>
  GetSerializedMessage() const;

  mojom::SolanaTxDataPtr ToSolanaTxData() const;
  base::Value::Dict ToValue() const;

  static std::unique_ptr<SolanaTransaction> FromSolanaTxData(
      mojom::SolanaTxDataPtr solana_tx_data);
  static std::unique_ptr<SolanaTransaction> FromValue(
      const base::Value::Dict& value);
  static std::unique_ptr<SolanaTransaction> FromSignedTransactionBytes(
      const std::vector<uint8_t>& bytes);

  void ClearRawSignatures() { raw_signatures_.clear(); }
  bool IsPartialSigned() const;

  std::string to_wallet_address() const { return to_wallet_address_; }
  std::string token_address() const { return token_address_; }
  mojom::TransactionType tx_type() const { return tx_type_; }
  uint64_t lamports() const { return lamports_; }
  uint64_t amount() const { return amount_; }
  SolanaMessage* message() { return &message_; }
  const std::vector<uint8_t>& raw_signatures() const { return raw_signatures_; }
  std::optional<SolanaTransaction::SendOptions> send_options() const {
    return send_options_;
  }
  const std::string& wired_tx() const { return wired_tx_; }

  void set_to_wallet_address(const std::string& to_wallet_address) {
    to_wallet_address_ = to_wallet_address;
  }
  void set_token_address(const std::string& token_address) {
    token_address_ = token_address;
  }
  void set_tx_type(mojom::TransactionType tx_type);
  void set_lamports(uint64_t lamports) { lamports_ = lamports; }
  void set_amount(uint64_t amount) { amount_ = amount; }
  void set_send_options(std::optional<SendOptions> options) {
    send_options_ = options;
  }
  void set_sign_tx_param(mojom::SolanaSignTransactionParamPtr sign_tx_param) {
    sign_tx_param_ = std::move(sign_tx_param);
  }
  void set_wired_tx(const std::string& wired_tx) { wired_tx_ = wired_tx; }
  void set_fee_estimation(mojom::SolanaFeeEstimationPtr estimation) {
    fee_estimation_ = std::move(estimation);
  }
  const mojom::SolanaFeeEstimationPtr& fee_estimation() const {
    return fee_estimation_;
  }

 private:
  FRIEND_TEST_ALL_PREFIXES(SolanaTransactionUnitTest, GetBase64EncodedMessage);

  SolanaMessage message_;
  // Value will be assigned when FromSignedTransactionBytes is called.
  std::vector<uint8_t> raw_signatures_;

  // Base64 encoded serialized transaction to be sent to the Solana network,
  // value will be assigned before we call
  // JsonRPCService::SendSolanaTransaction and reused when we rebroadcast the
  // transaction.
  std::string wired_tx_;

  // Passed by dApp when calling signAndSendTransaction, signTransaction,
  // signAllTransactions provider APIs, which includes serialized message and
  // signatures from partial_sign. If this exists, we will use the serialized
  // message inside when signing the transaction instead of serializing the
  // message by ourselves. This is the order of accounts with the same
  // is_signer and is_writable properties can be different across
  // implementations, we need to sign the exact serialized message being passed
  // by dApp.
  mojom::SolanaSignTransactionParamPtr sign_tx_param_ = nullptr;

  // Data fields to be used for UI, they are filled currently when we create
  // SolanaTxData to transfer SOL or SPL tokens for UI.
  std::string to_wallet_address_;
  std::string token_address_;
  mojom::TransactionType tx_type_ = mojom::TransactionType::Other;
  uint64_t lamports_ = 0;  // amount of lamports to transfer
  uint64_t amount_ = 0;    // amount of SPL tokens to transfer

  // Currently might be specified by solana.signAndSendTransaction provider
  // API as the options to be passed to sendTransaction RPC call.
  std::optional<SendOptions> send_options_;

  // Fee estimation result
  mojom::SolanaFeeEstimationPtr fee_estimation_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_TRANSACTION_H_
