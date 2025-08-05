/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_provider_impl.h"

#include <utility>
#include <vector>

#include "base/notimplemented.h"
#include "base/notreached.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/brave_wallet_provider_delegate.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_api_impl.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_dapp_utils.h"
#include "brave/components/brave_wallet/common/cardano_address.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "components/grit/brave_components_strings.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

namespace {
constexpr char kTabNotVisibleError[] = "Tab not active";

// APIErrorCode
constexpr int kAPIErrorInternalError = -2;
constexpr int kAPIErrorRefused = -3;

}  // namespace

CardanoProviderImpl::CardanoProviderImpl(
    BraveWalletService& brave_wallet_service,
    BraveWalletProviderDelegateFactory delegate_factory)
    : brave_wallet_service_(brave_wallet_service),
      delegate_factory_(std::move(delegate_factory)) {
  brave_wallet_service_->keyring_service()->AddObserver(
      keyring_observer_receiver_.BindNewPipeAndPassRemote());
  delegate_ = delegate_factory_.Run();
  CHECK(delegate_);
}

CardanoProviderImpl::~CardanoProviderImpl() = default;

// mojom::CardanoProvider
void CardanoProviderImpl::IsEnabled(IsEnabledCallback callback) {
  NOTIMPLEMENTED_LOG_ONCE();

  // Mocked values for development usage.
  std::move(callback).Run(true);
}

void CardanoProviderImpl::Enable(
    mojo::PendingReceiver<mojom::CardanoApi> cardano_api,
    EnableCallback callback) {
  if (!delegate_->IsTabVisible()) {
    std::move(callback).Run(mojom::CardanoProviderErrorBundle::New(
        kAPIErrorRefused, kTabNotVisibleError, nullptr));
    return;
  }

  delegate_->WalletInteractionDetected();

  RequestCardanoPermissions(std::move(cardano_api), std::move(callback),
                            delegate_->GetOrigin());
}

void CardanoProviderImpl::RequestCardanoPermissions(
    mojo::PendingReceiver<mojom::CardanoApi> cardano_api,
    EnableCallback callback,
    const url::Origin& origin) {
  if (delegate_->IsPermissionDenied(mojom::CoinType::ADA)) {
    std::move(callback).Run(mojom::CardanoProviderErrorBundle::New(
        kAPIErrorRefused,
        l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST), nullptr));
    return;
  }

  if (!brave_wallet_service_->keyring_service()->IsWalletCreatedSync()) {
    if (!wallet_page_shown_) {
      delegate_->ShowWalletOnboarding();
      wallet_page_shown_ = true;
    }
    std::move(callback).Run(mojom::CardanoProviderErrorBundle::New(
        kAPIErrorRefused,
        l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST), nullptr));
    return;
  }

  auto cardano_account_ids = GetCardanoAccountPermissionIdentifiers(
      brave_wallet_service_->keyring_service());

  if (cardano_account_ids.empty()) {
    if (!wallet_page_shown_) {
      delegate_->ShowAccountCreation(mojom::CoinType::ADA);
      wallet_page_shown_ = true;
    }
    std::move(callback).Run(mojom::CardanoProviderErrorBundle::New(
        kAPIErrorRefused,
        l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST), nullptr));
    return;
  }

  if (brave_wallet_service_->keyring_service()->IsLockedSync()) {
    // If there already was a request to unlock the wallet we drop pending
    // requests.
    if (pending_request_cardano_permissions_callback_) {
      std::move(callback).Run(mojom::CardanoProviderErrorBundle::New(
          kAPIErrorRefused,
          l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST), nullptr));
      return;
    }
    pending_cardano_api_ = std::move(cardano_api);
    pending_request_cardano_permissions_callback_ = std::move(callback);
    pending_request_cardano_permissions_origin_ = origin;

    brave_wallet_service_->keyring_service()->RequestUnlock();
    delegate_->ShowPanel();
    return;
  }

  const auto allowed_accounts =
      delegate_->GetAllowedAccounts(mojom::CoinType::ADA, cardano_account_ids);
  const bool success = allowed_accounts.has_value();

  if (!success) {
    std::move(callback).Run(mojom::CardanoProviderErrorBundle::New(
        kAPIErrorInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST), nullptr));
    return;
  }

  if (success && !allowed_accounts->empty()) {
    OnRequestCardanoPermissions(std::move(cardano_api), std::move(callback),
                                origin, mojom::RequestPermissionsError::kNone,
                                allowed_accounts);
  } else {
    // Request accounts if no accounts are connected.
    delegate_->RequestPermissions(
        mojom::CoinType::ADA, cardano_account_ids,
        base::BindOnce(&CardanoProviderImpl::OnRequestCardanoPermissions,
                       weak_ptr_factory_.GetWeakPtr(), std::move(cardano_api),
                       std::move(callback), origin));
  }
}

void CardanoProviderImpl::OnRequestCardanoPermissions(
    mojo::PendingReceiver<mojom::CardanoApi> cardano_api,
    EnableCallback callback,
    const url::Origin& origin,
    mojom::RequestPermissionsError error,
    const std::optional<std::vector<std::string>>& allowed_accounts) {
  bool success = error == mojom::RequestPermissionsError::kNone;

  if (!success) {
    switch (error) {
      case mojom::RequestPermissionsError::kRequestInProgress:
        std::move(callback).Run(mojom::CardanoProviderErrorBundle::New(
            kAPIErrorRefused,
            l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST),
            nullptr));
        return;
      case mojom::RequestPermissionsError::kInternal:
        std::move(callback).Run(mojom::CardanoProviderErrorBundle::New(
            kAPIErrorInternalError,
            l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST),
            nullptr));
        return;
      default:
        NOTREACHED() << error;
    }
  }

  auto account_id = GetCardanoAllowedSelectedAccount(
      delegate(), brave_wallet_service_->keyring_service());

  if (!account_id) {
    std::move(callback).Run(mojom::CardanoProviderErrorBundle::New(
        kAPIErrorRefused,
        l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST), nullptr));
  }

  auto api_impl = std::make_unique<CardanoApiImpl>(
      brave_wallet_service_.get(), delegate_factory_.Run(), account_id.Clone());
  mojo::MakeSelfOwnedReceiver(std::move(api_impl), std::move(cardano_api));
  std::move(callback).Run(nullptr);
}

BraveWalletProviderDelegate* CardanoProviderImpl::delegate() {
  return delegate_.get();
}

void CardanoProviderImpl::Locked() {}

void CardanoProviderImpl::Unlocked() {
  if (pending_request_cardano_permissions_callback_) {
    RequestCardanoPermissions(
        std::move(pending_cardano_api_),
        std::move(pending_request_cardano_permissions_callback_),
        std::move(pending_request_cardano_permissions_origin_));
  }
}

void CardanoProviderImpl::SelectedDappAccountChanged(
    mojom::CoinType coin,
    mojom::AccountInfoPtr account) {
  // CardanoApiImpl will handle this case itself
}

}  // namespace brave_wallet
