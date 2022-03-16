/* copyright (c) 2022 the brave authors. all rights reserved.
 * this source code form is subject to the terms of the mozilla public
 * license, v. 2.0. if a copy of the mpl was not distributed with this file,
 * you can obtain one at http://mozilla.org/mpl/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_ACCOUNT_META_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_ACCOUNT_META_H_

#include <string>

namespace brave_wallet {

struct SolanaAccountMeta {
  SolanaAccountMeta(const std::string& pubkey,
                    bool is_signer,
                    bool is_writable);
  ~SolanaAccountMeta();

  SolanaAccountMeta(const SolanaAccountMeta&);
  bool operator==(const SolanaAccountMeta&) const;

  std::string pubkey;
  bool is_signer;
  bool is_writable;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_ACCOUNT_META_H_
