/* copyright (c) 2022 the brave authors. all rights reserved.
 * this source code form is subject to the terms of the mozilla public
 * license, v. 2.0. if a copy of the mpl was not distributed with this file,
 * you can obtain one at http://mozilla.org/mpl/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_TRANSACTION_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_TRANSACTION_H_

#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/solana_message.h"

namespace brave_wallet {

class KeyringService;
class SolanaInstruction;

class SolanaTransaction {
 public:
  SolanaTransaction(const std::string& recent_blockhash,
                    const std::string& fee_payer,
                    const std::vector<SolanaInstruction>& instructions);
  SolanaTransaction(const SolanaTransaction&) = default;
  ~SolanaTransaction() = default;

  // Serialize the message and sign it.
  std::string GetSignedTransaction(KeyringService* keyring_service,
                                   const std::string& recent_blockhash);

 private:
  SolanaMessage message_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_TRANSACTION_H_
