/* copyright (c) 2022 the brave authors. all rights reserved.
 * this source code form is subject to the terms of the mozilla public
 * license, v. 2.0. if a copy of the mpl was not distributed with this file,
 * you can obtain one at http://mozilla.org/mpl/2.0/. */

#include "brave/components/brave_wallet/browser/solana_account_meta.h"

#include "base/values.h"

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

mojom::SolanaAccountMetaPtr SolanaAccountMeta::ToMojomSolanaAccountMeta()
    const {
  return mojom::SolanaAccountMeta::New(pubkey, is_signer, is_writable);
}

base::Value SolanaAccountMeta::ToValue() const {
  base::Value dict(base::Value::Type::DICTIONARY);
  dict.SetStringKey("pubkey", pubkey);
  dict.SetBoolKey("is_signer", is_signer);
  dict.SetBoolKey("is_writable", is_writable);
  return dict;
}

// static
absl::optional<SolanaAccountMeta> SolanaAccountMeta::FromValue(
    const base::Value& value) {
  if (!value.is_dict())
    return absl::nullopt;

  const std::string* pubkey = value.FindStringKey("pubkey");
  if (!pubkey)
    return absl::nullopt;
  absl::optional<bool> is_signer = value.FindBoolKey("is_signer");
  if (!is_signer)
    return absl::nullopt;
  absl::optional<bool> is_writable = value.FindBoolKey("is_writable");
  if (!is_writable)
    return absl::nullopt;

  return SolanaAccountMeta(*pubkey, *is_signer, *is_writable);
}

// static
void SolanaAccountMeta::FromMojomSolanaAccountMetas(
    const std::vector<mojom::SolanaAccountMetaPtr>& mojom_account_metas,
    std::vector<SolanaAccountMeta>* account_metas) {
  if (!account_metas)
    return;
  account_metas->clear();
  for (const auto& mojom_account_meta : mojom_account_metas)
    account_metas->push_back(SolanaAccountMeta(
        mojom_account_meta->pubkey, mojom_account_meta->is_signer,
        mojom_account_meta->is_writable));
}

}  // namespace brave_wallet
