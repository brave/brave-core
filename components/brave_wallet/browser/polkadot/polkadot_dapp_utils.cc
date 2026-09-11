/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_dapp_utils.h"

#include <algorithm>

#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/common/common_utils.h"

namespace brave_wallet {

namespace {

// Every Polkadot account we hold is an sr25519 keypair. `KeypairType` in
// @polkadot/util-crypto.
constexpr char kSr25519[] = "sr25519";

}  // namespace

std::vector<std::string> GetPolkadotAccountPermissionIdentifiers(
    KeyringService* keyring_service) {
  std::vector<std::string> ids;
  for (const auto& account : keyring_service->GetAllAccountInfos()) {
    if (account && account->account_id &&
        IsPolkadotAccount(account->account_id)) {
      ids.push_back(GetAccountPermissionIdentifier(account->account_id));
    }
  }
  return ids;
}

mojom::AccountIdPtr GetPolkadotPreferredDappAccount(
    KeyringService* keyring_service,
    const std::optional<std::vector<std::string>>& allowed_accounts) {
  if (!allowed_accounts || allowed_accounts->empty()) {
    return nullptr;
  }

  auto selected_account = keyring_service->GetSelectedPolkadotDappAccount();
  if (selected_account &&
      std::ranges::contains(
          *allowed_accounts,
          GetAccountPermissionIdentifier(selected_account->account_id))) {
    return selected_account->account_id.Clone();
  }

  // The connect prompt grants a single account, so the selected dapp account
  // normally is the allowed one. Fall back to the first allowed account when it
  // isn't, matching CardanoProviderImpl and EthereumProviderImpl.
  for (const auto& account : keyring_service->GetAllAccountInfos()) {
    if (std::ranges::contains(
            *allowed_accounts,
            GetAccountPermissionIdentifier(account->account_id))) {
      return account->account_id.Clone();
    }
  }
  return nullptr;
}

mojom::PolkadotInjectedAccountPtr MakePolkadotInjectedAccount(
    const mojom::AccountInfo& account) {
  // `genesis_hash` is left unset, which dapps read as "usable on any chain".
  // The address we hand over is encoded with the keyring's SS58 prefix, and
  // @polkadot/extension-dapp re-encodes it with the prefix of the chain the
  // dapp is on, so the prefix we pick here is not what the dapp displays.
  // Restricting testnet accounts to their chain needs the chain's genesis
  // hash, which isn't available synchronously here.
  return mojom::PolkadotInjectedAccount::New(
      account.address, /*genesis_hash=*/std::nullopt, account.name, kSr25519);
}

}  // namespace brave_wallet
