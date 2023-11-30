/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_ACCOUNT_META_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_ACCOUNT_META_H_

#include <optional>
#include <string>
#include <vector>

#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

struct SolanaAccountMeta {
  SolanaAccountMeta(const std::string& pubkey,
                    std::optional<uint8_t> address_table_lookup_index,
                    bool is_signer,
                    bool is_writable);
  ~SolanaAccountMeta();

  SolanaAccountMeta(const SolanaAccountMeta&);
  bool operator==(const SolanaAccountMeta&) const;

  mojom::SolanaAccountMetaPtr ToMojomSolanaAccountMeta() const;
  base::Value::Dict ToValue() const;

  static void FromMojomSolanaAccountMetas(
      const std::vector<mojom::SolanaAccountMetaPtr>& mojom_account_metas,
      std::vector<SolanaAccountMeta>* account_metas);
  static std::optional<SolanaAccountMeta> FromValue(
      const base::Value::Dict& value);

  std::string pubkey;
  std::optional<uint8_t> address_table_lookup_index;
  bool is_signer;
  bool is_writable;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_ACCOUNT_META_H_
