/* copyright (c) 2022 the brave authors. all rights reserved.
 * this source code form is subject to the terms of the mozilla public
 * license, v. 2.0. if a copy of the mpl was not distributed with this file,
 * you can obtain one at http://mozilla.org/mpl/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_ACCOUNT_META_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_ACCOUNT_META_H_

#include <string>
#include <vector>

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace base {
class Value;
}  // namespace base

namespace brave_wallet {

struct SolanaAccountMeta {
  SolanaAccountMeta(const std::string& pubkey,
                    bool is_signer,
                    bool is_writable);
  ~SolanaAccountMeta();

  SolanaAccountMeta(const SolanaAccountMeta&);
  bool operator==(const SolanaAccountMeta&) const;

  mojom::SolanaAccountMetaPtr ToMojomSolanaAccountMeta() const;
  base::Value ToValue() const;

  static void FromMojomSolanaAccountMetas(
      const std::vector<mojom::SolanaAccountMetaPtr>& mojom_account_metas,
      std::vector<SolanaAccountMeta>* account_metas);
  static absl::optional<SolanaAccountMeta> FromValue(const base::Value& value);

  std::string pubkey;
  bool is_signer;
  bool is_writable;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_ACCOUNT_META_H_
