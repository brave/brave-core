/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_dapp_utils.h"

#include <algorithm>

#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"

namespace brave_wallet {

namespace {

// `MultiSignature::Sr25519`'s variant index. Our Polkadot keyrings only hold
// sr25519 keypairs.
// https://github.com/polkadot-js/api/blob/master/packages/types/src/interfaces/extrinsics/definitions.ts
inline constexpr uint8_t kSr25519MultiSignatureIndex = 1;

bool IsPolkadotDappAccount(const mojom::AccountIdPtr& account_id) {
  return account_id->keyring_id == mojom::KeyringId::kPolkadotMainnet ||
         account_id->keyring_id == mojom::KeyringId::kPolkadotImport;
}

}  // namespace

std::vector<mojom::AccountIdPtr> GetPolkadotDappAccountIds(
    KeyringService* keyring_service) {
  std::vector<mojom::AccountIdPtr> ids;
  for (const auto& account : keyring_service->GetAllAccountInfos()) {
    if (account && account->account_id &&
        IsPolkadotDappAccount(account->account_id)) {
      ids.push_back(account->account_id.Clone());
    }
  }
  return ids;
}

mojom::AccountIdPtr GetPolkadotPreferredDappAccount(
    KeyringService* keyring_service,
    const std::vector<mojom::AccountIdPtr>& allowed_accounts) {
  if (allowed_accounts.empty()) {
    return nullptr;
  }

  auto selected_account = keyring_service->GetSelectedPolkadotDappAccount();
  if (selected_account &&
      std::ranges::contains(allowed_accounts, selected_account->account_id)) {
    return selected_account->account_id.Clone();
  }

  for (const auto& account : keyring_service->GetAllAccountInfos()) {
    if (!IsPolkadotDappAccount(account->account_id)) {
      continue;
    }

    if (std::ranges::contains(allowed_accounts, account->account_id)) {
      return account->account_id.Clone();
    }
  }
  return nullptr;
}

mojom::PolkadotInjectedAccountPtr MakePolkadotInjectedAccount(
    const mojom::AccountInfo& account) {
  return mojom::PolkadotInjectedAccount::New(
      account.address, /*genesis_hash=*/std::nullopt,
      kPolkadotObfuscatedAccountName, kPolkadotSr25519KeypairType);
}

std::string MakePolkadotSignerSignatureHex(
    base::span<const uint8_t, kSr25519SignatureSize> signature) {
  return base::StrCat(
      {"0x",
       base::HexEncodeLower(base::span_from_ref(kSr25519MultiSignatureIndex)),
       base::HexEncodeLower(signature)});
}

}  // namespace brave_wallet
