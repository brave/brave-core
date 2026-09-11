/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_api_impl.h"

#include <optional>
#include <utility>
#include <vector>

#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_dapp_utils.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

PolkadotApiImpl::PolkadotApiImpl(
    BraveWalletService& brave_wallet_service,
    std::unique_ptr<BraveWalletProviderDelegate> delegate,
    mojom::AccountIdPtr granted_account)
    : brave_wallet_service_(brave_wallet_service),
      delegate_(std::move(delegate)),
      granted_account_(std::move(granted_account)) {
  CHECK(delegate_);
  CHECK(granted_account_);
}

PolkadotApiImpl::~PolkadotApiImpl() = default;

bool PolkadotApiImpl::IsGrantedAccountAllowed() {
  return delegate_->IsAccountAllowed(
      mojom::CoinType::DOT, GetAccountPermissionIdentifier(granted_account_));
}

void PolkadotApiImpl::GetAccounts(bool any_type,
                                  GetAccountsCallback callback) {
  // `any_type` asks for accounts that can't sign either. Every account we can
  // hand over is a signing sr25519 account, so the flag makes no difference.

  if (!IsGrantedAccountAllowed()) {
    std::move(callback).Run(
        std::nullopt,
        mojom::PolkadotProviderErrorBundle::New(
            mojom::PolkadotProviderError::kUnknown,
            l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST)));
    return;
  }

  auto* keyring_service = brave_wallet_service_->keyring_service();
  if (keyring_service->IsLockedSync()) {
    std::move(callback).Run(
        std::nullopt,
        mojom::PolkadotProviderErrorBundle::New(
            mojom::PolkadotProviderError::kUnknown,
            l10n_util::GetStringUTF8(IDS_WALLET_REQUEST_PROCESSING_ERROR)));
    return;
  }

  auto account = keyring_service->FindAccount(granted_account_);
  if (!account) {
    std::move(callback).Run(
        std::nullopt,
        mojom::PolkadotProviderErrorBundle::New(
            mojom::PolkadotProviderError::kInternalError,
            l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)));
    return;
  }

  // A connect prompt grants exactly one account, so this list always holds a
  // single entry. Dapps treat it as a list either way.
  std::vector<mojom::PolkadotInjectedAccountPtr> accounts;
  accounts.push_back(MakePolkadotInjectedAccount(*account));
  std::move(callback).Run(std::move(accounts), nullptr);
}

}  // namespace brave_wallet
