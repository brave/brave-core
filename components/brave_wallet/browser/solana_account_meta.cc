/* copyright (c) 2022 the brave authors. all rights reserved.
 * this source code form is subject to the terms of the mozilla public
 * license, v. 2.0. if a copy of the mpl was not distributed with this file,
 * you can obtain one at http://mozilla.org/mpl/2.0/. */

#include "brave/components/brave_wallet/browser/solana_account_meta.h"

namespace brave_wallet {

SolanaAccountMeta::SolanaAccountMeta(const std::string& pubkey,
                                     bool is_signer,
                                     bool is_writable)
    : pubkey(pubkey), is_signer(is_signer), is_writable(is_writable) {}

SolanaAccountMeta::SolanaAccountMeta(const SolanaAccountMeta&) = default;

SolanaAccountMeta::~SolanaAccountMeta() = default;

bool SolanaAccountMeta::operator==(const SolanaAccountMeta& meta) const {
  return pubkey == meta.pubkey && is_signer == meta.is_signer &&
         is_writable == meta.is_writable;
}

}  // namespace brave_wallet
