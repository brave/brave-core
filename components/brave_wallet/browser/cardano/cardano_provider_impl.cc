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
  std::vector<std::string> allowed_accounts;
  auto result = EvaluatePermissionsState(allowed_accounts);
  std::move(callback).Run(result == PermissionCheckResult::kHasAllowedAccounts);
}

void CardanoProviderImpl::Enable(
    mojo::PendingReceiver<mojom::CardanoApi> cardano_api,
    EnableCallback callback) {
  RequestCardanoPermissions(std::move(cardano_api), std::move(callback),
                            delegate_->GetOrigin());
}

void CardanoProviderImpl::RequestCardanoPermissions(
    mojo::PendingReceiver<mojom::CardanoApi> cardano_api,
    EnableCallback callback,
    const url::Origin& origin) {
  std::vector<std::string> allowed_accounts;
  auto state = EvaluatePermissionsState(allowed_accounts);

  switch (state) {
    case PermissionCheckResult::kTabInactive:
      return std::move(callback).Run(mojom::CardanoProviderErrorBundle::New(
          kAPIErrorRefused, kTabNotVisibleError, nullptr));

    case PermissionCheckResult::kDeniedGlobally:
      return std::move(callback).Run(mojom::CardanoProviderErrorBundle::New(
          kAPIErrorRefused,
          l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST), nullptr));

    case PermissionCheckResult::kWalletNotCreated:
      if (!wallet_page_shown_) {
        delegate_->ShowWalletOnboarding();
        wallet_page_shown_ = true;
      }
      std::move(callback).Run(mojom::CardanoProviderErrorBundle::New(
          kAPIErrorRefused,
          l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST), nullptr));
      return;

    case PermissionCheckResult::kNoAccounts:
      if (!wallet_page_shown_) {
        delegate_->ShowAccountCreation(mojom::CoinType::ADA);
        wallet_page_shown_ = true;
      }
      std::move(callback).Run(mojom::CardanoProviderErrorBundle::New(
          kAPIErrorRefused,
          l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST), nullptr));
      return;

    case PermissionCheckResult::kWalletLocked:
      // If there already was a request to unlock the wallet we drop pending
      // requests.
      if (pending_request_cardano_permissions_callback_) {
        std::move(callback).Run(mojom::CardanoProviderErrorBundle::New(
            kAPIErrorRefused,
            l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST),
            nullptr));
        return;
      }
      pending_cardano_api_ = std::move(cardano_api);
      pending_request_cardano_permissions_callback_ = std::move(callback);
      pending_request_cardano_permissions_origin_ = origin;

      brave_wallet_service_->keyring_service()->RequestUnlock();
      delegate_->ShowPanel();
      return;

    case PermissionCheckResult::kGetAllowedAccountsFailed:
      return std::move(callback).Run(mojom::CardanoProviderErrorBundle::New(
          kAPIErrorInternalError,
          l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST), nullptr));

    case PermissionCheckResult::kHasAllowedAccounts:
      return OnRequestCardanoPermissions(
          std::move(cardano_api), std::move(callback), origin,
          mojom::RequestPermissionsError::kNone, allowed_accounts);

    case PermissionCheckResult::kNeedsPermissionRequest:
      auto cardano_account_ids = GetCardanoAccountPermissionIdentifiers(
          brave_wallet_service_->keyring_service());
      return delegate_->RequestPermissions(
          mojom::CoinType::ADA, cardano_account_ids,
          base::BindOnce(&CardanoProviderImpl::OnRequestCardanoPermissions,
                         weak_ptr_factory_.GetWeakPtr(), std::move(cardano_api),
                         std::move(callback), origin));
  }
}

CardanoProviderImpl::PermissionCheckResult
CardanoProviderImpl::EvaluatePermissionsState(
    std::vector<std::string>& allowed_accounts) {
  if (!delegate_->IsTabVisible()) {
    return PermissionCheckResult::kTabInactive;
  }

  delegate_->WalletInteractionDetected();

  if (delegate_->IsPermissionDenied(mojom::CoinType::ADA)) {
    return PermissionCheckResult::kDeniedGlobally;
  }

  if (!brave_wallet_service_->keyring_service()->IsWalletCreatedSync()) {
    return PermissionCheckResult::kWalletNotCreated;
  }

  auto cardano_account_ids = GetCardanoAccountPermissionIdentifiers(
      brave_wallet_service_->keyring_service());

  if (cardano_account_ids.empty()) {
    return PermissionCheckResult::kNoAccounts;
  }

  if (brave_wallet_service_->keyring_service()->IsLockedSync()) {
    return PermissionCheckResult::kWalletLocked;
  }

  auto allowed_accounts_value =
      delegate_->GetAllowedAccounts(mojom::CoinType::ADA, cardano_account_ids);

  if (!allowed_accounts_value) {
    return PermissionCheckResult::kGetAllowedAccountsFailed;
  }

  if (allowed_accounts_value->empty()) {
    return PermissionCheckResult::kNeedsPermissionRequest;
  }

  allowed_accounts = *allowed_accounts_value;

  return PermissionCheckResult::kHasAllowedAccounts;
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

  auto account_id = GetCardanoPreferredDappAccount(
      delegate(), brave_wallet_service_->keyring_service());

  if (!account_id) {
    std::move(callback).Run(mojom::CardanoProviderErrorBundle::New(
        kAPIErrorRefused,
        l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST), nullptr));
    return;
  }

  cardano_api_receivers_.Add(std::make_unique<CardanoApiImpl>(
                                 brave_wallet_service_.get(),
                                 delegate_factory_.Run(), account_id.Clone()),
                             std::move(cardano_api));

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
