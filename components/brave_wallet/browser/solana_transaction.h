/* copyright (c) 2022 the brave authors. all rights reserved.
 * this source code form is subject to the terms of the mozilla public
 * license, v. 2.0. if a copy of the mpl was not distributed with this file,
 * you can obtain one at http://mozilla.org/mpl/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_TRANSACTION_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_TRANSACTION_H_

#include <memory>
#include <string>
#include <vector>

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
                    const std::string& fee_payer,
                    std::vector<SolanaInstruction>&& instructions);
  SolanaTransaction(const SolanaTransaction&) = default;
  ~SolanaTransaction() = default;
  bool operator==(const SolanaTransaction&) const;

  // Serialize the message and sign it.
  std::string GetSignedTransaction(KeyringService* keyring_service,
                                   const std::string& recent_blockhash);

  mojom::SolanaTxDataPtr ToSolanaTxData() const;
  base::Value ToValue() const;

  static std::unique_ptr<SolanaTransaction> FromSolanaTxData(
      mojom::SolanaTxDataPtr solana_tx_data);
  static absl::optional<SolanaTransaction> FromValue(const base::Value& value);

 private:
  SolanaMessage message_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_TRANSACTION_H_
