/* copyright (c) 2022 the brave authors. all rights reserved.
 * this source code form is subject to the terms of the mozilla public
 * license, v. 2.0. if a copy of the mpl was not distributed with this file,
 * you can obtain one at http://mozilla.org/mpl/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_TRANSACTION_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_TRANSACTION_H_

#include <memory>
#include <string>
#include <vector>

#include "base/gtest_prod_util.h"
#include "brave/components/brave_wallet/browser/solana_message.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace base {
class Value;
}  // namespace base

namespace brave_wallet {

class KeyringService;
class SolanaInstruction;

class SolanaTransaction {
 public:
  explicit SolanaTransaction(SolanaMessage&& message);
  SolanaTransaction(const std::string& recent_blockhash,
                    uint64_t last_valid_block_height,
                    const std::string& fee_payer,
                    std::vector<SolanaInstruction>&& instructions);
  SolanaTransaction(const SolanaTransaction&);
  ~SolanaTransaction();
  bool operator==(const SolanaTransaction&) const;

  // Serialize the message and sign it.
  std::string GetSignedTransaction(KeyringService* keyring_service) const;
  // Serialize and encode the message in Base64.
  std::string GetBase64EncodedMessage() const;

  mojom::SolanaTxDataPtr ToSolanaTxData() const;
  base::Value ToValue() const;

  static std::unique_ptr<SolanaTransaction> FromSolanaTxData(
      mojom::SolanaTxDataPtr solana_tx_data);
  static absl::optional<SolanaTransaction> FromValue(const base::Value& value);

  std::string to_wallet_address() const { return to_wallet_address_; }
  std::string spl_token_mint_address() const { return spl_token_mint_address_; }
  mojom::TransactionType tx_type() const { return tx_type_; }
  uint64_t lamports() const { return lamports_; }
  uint64_t amount() const { return amount_; }
  SolanaMessage* message() { return &message_; }

  void set_to_wallet_address(const std::string& to_wallet_address) {
    to_wallet_address_ = to_wallet_address;
  }
  void set_spl_token_mint_address(const std::string& spl_token_mint_address) {
    spl_token_mint_address_ = spl_token_mint_address;
  }
  void set_tx_type(mojom::TransactionType tx_type);
  void set_lamports(uint64_t lamports) { lamports_ = lamports; }
  void set_amount(uint64_t amount) { amount_ = amount; }

 private:
  FRIEND_TEST_ALL_PREFIXES(SolanaTransactionUnitTest, GetBase64EncodedMessage);
  SolanaMessage message_;

  // Data fields to be used for UI, they are filled currently when we create
  // SolanaTxData to transfer SOL or SPL tokens for UI.
  std::string to_wallet_address_;
  std::string spl_token_mint_address_;
  mojom::TransactionType tx_type_ = mojom::TransactionType::Other;
  uint64_t lamports_ = 0;  // amount of lamports to transfer
  uint64_t amount_ = 0;    // amount of SPL tokens to transfer
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_TRANSACTION_H_
