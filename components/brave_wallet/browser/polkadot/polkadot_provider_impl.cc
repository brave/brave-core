/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_provider_impl.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/notreached.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_api_impl.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_dapp_utils.h"
#include "components/grit/brave_components_strings.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

namespace {

mojom::PolkadotProviderErrorBundlePtr RejectedError() {
  return mojom::PolkadotProviderErrorBundle::New(
      mojom::PolkadotProviderError::kUnknown,
      l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));
}

mojom::PolkadotProviderErrorBundlePtr InternalError() {
  return mojom::PolkadotProviderErrorBundle::New(
      mojom::PolkadotProviderError::kInternalError,
      l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
}

}  // namespace

PolkadotProviderImpl::PolkadotProviderImpl(
    BraveWalletService& brave_wallet_service,
    BraveWalletProviderDelegateFactory delegate_factory,
    const url::Origin& origin)
    : brave_wallet_service_(brave_wallet_service),
      delegate_factory_(std::move(delegate_factory)),
      origin_(origin) {
  brave_wallet_service_->keyring_service()->AddObserver(
      keyring_observer_receiver_.BindNewPipeAndPassRemote());
  delegate_ = delegate_factory_.Run();
  CHECK(delegate_);
}

PolkadotProviderImpl::~PolkadotProviderImpl() = default;

void PolkadotProviderImpl::Enable(EnableCallback callback) {
  delegate_->WalletInteractionDetected();

  RequestPolkadotPermissions(std::move(callback), origin_);
}

void PolkadotProviderImpl::RequestPolkadotPermissions(
    EnableCallback callback,
    const url::Origin& origin) {
  std::vector<std::string> allowed_accounts;
  auto state = EvaluatePermissionsState(allowed_accounts);

  switch (state) {
    case PermissionCheckResult::kTabInactive:
      return std::move(callback).Run(
          mojo::NullRemote(),
          mojom::PolkadotProviderErrorBundle::New(
              mojom::PolkadotProviderError::kUnknown,
              l10n_util::GetStringUTF8(IDS_WALLET_TAB_IS_NOT_ACTIVE_ERROR)));

    case PermissionCheckResult::kDeniedGlobally:
      return std::move(callback).Run(mojo::NullRemote(), RejectedError());

    // The provider is only injected once a wallet exists, so this is a
    // defensive case (e.g. the wallet was reset while the page stayed open).
    case PermissionCheckResult::kWalletNotCreated:
      return std::move(callback).Run(mojo::NullRemote(), RejectedError());

    case PermissionCheckResult::kNoAccounts:
      if (!account_creation_shown_) {
        delegate_->ShowAccountCreation(mojom::CoinType::DOT, origin_);
        account_creation_shown_ = true;
      }
      return std::move(callback).Run(mojo::NullRemote(), RejectedError());

    case PermissionCheckResult::kWalletLocked:
      // If there already was a request to unlock the wallet we drop pending
      // requests.
      if (pending_request_permissions_callback_) {
        return std::move(callback).Run(mojo::NullRemote(), RejectedError());
      }
      pending_request_permissions_callback_ = std::move(callback);
      pending_request_permissions_origin_ = origin;

      brave_wallet_service_->keyring_service()->RequestUnlock();
      delegate_->ShowPanel(origin_);
      return;

    case PermissionCheckResult::kGetAllowedAccountsFailed:
      return std::move(callback).Run(mojo::NullRemote(), InternalError());

    case PermissionCheckResult::kHasAllowedAccounts:
      return OnRequestPolkadotPermissions(
          std::move(callback), mojom::RequestPermissionsError::kNone,
          allowed_accounts);

    case PermissionCheckResult::kNeedsPermissionRequest:
      auto polkadot_account_ids = GetPolkadotAccountPermissionIdentifiers(
          brave_wallet_service_->keyring_service());
      return delegate_->RequestPermissions(
          mojom::CoinType::DOT, polkadot_account_ids, origin,
          base::BindOnce(&PolkadotProviderImpl::OnRequestPolkadotPermissions,
                         weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
  }
}

PolkadotProviderImpl::PermissionCheckResult
PolkadotProviderImpl::EvaluatePermissionsState(
    std::vector<std::string>& allowed_accounts) {
  if (!delegate_->IsTabVisible()) {
    return PermissionCheckResult::kTabInactive;
  }

  if (delegate_->IsPermissionDenied(mojom::CoinType::DOT)) {
    return PermissionCheckResult::kDeniedGlobally;
  }

  auto* keyring_service = brave_wallet_service_->keyring_service();
  if (!keyring_service->IsWalletCreatedSync()) {
    return PermissionCheckResult::kWalletNotCreated;
  }

  auto polkadot_account_ids =
      GetPolkadotAccountPermissionIdentifiers(keyring_service);
  if (polkadot_account_ids.empty()) {
    return PermissionCheckResult::kNoAccounts;
  }

  if (keyring_service->IsLockedSync()) {
    return PermissionCheckResult::kWalletLocked;
  }

  auto allowed_accounts_value = delegate_->GetAllowedAccounts(
      mojom::CoinType::DOT, polkadot_account_ids);
  if (!allowed_accounts_value) {
    return PermissionCheckResult::kGetAllowedAccountsFailed;
  }

  if (allowed_accounts_value->empty()) {
    return PermissionCheckResult::kNeedsPermissionRequest;
  }

  allowed_accounts = *allowed_accounts_value;

  return PermissionCheckResult::kHasAllowedAccounts;
}

void PolkadotProviderImpl::OnRequestPolkadotPermissions(
    EnableCallback callback,
    mojom::RequestPermissionsError error,
    const std::optional<std::vector<std::string>>& allowed_accounts) {
  if (error != mojom::RequestPermissionsError::kNone) {
    switch (error) {
      case mojom::RequestPermissionsError::kRequestInProgress:
        return std::move(callback).Run(mojo::NullRemote(), RejectedError());
      case mojom::RequestPermissionsError::kInternal:
        return std::move(callback).Run(mojo::NullRemote(), InternalError());
      default:
        NOTREACHED() << error;
    }
  }

  // `allowed_accounts` is fed straight into GetPolkadotPreferredDappAccount
  // instead of being queried again from the delegate: on iOS the front-end
  // database write may not have completed yet.
  auto account_id = GetPolkadotPreferredDappAccount(
      brave_wallet_service_->keyring_service(), allowed_accounts);

  if (!account_id) {
    return std::move(callback).Run(mojo::NullRemote(), RejectedError());
  }

  mojo::PendingRemote<mojom::PolkadotApi> polkadot_api_remote;
  polkadot_api_receivers_.Add(
      std::make_unique<PolkadotApiImpl>(brave_wallet_service_.get(),
                                        delegate_factory_.Run(),
                                        std::move(account_id)),
      polkadot_api_remote.InitWithNewPipeAndPassReceiver());

  std::move(callback).Run(std::move(polkadot_api_remote), nullptr);
}

void PolkadotProviderImpl::Unlocked() {
  if (pending_request_permissions_callback_) {
    RequestPolkadotPermissions(
        std::move(pending_request_permissions_callback_),
        std::move(pending_request_permissions_origin_));
  }
}

}  // namespace brave_wallet
