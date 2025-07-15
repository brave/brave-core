/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_provider_impl.h"

#include <utility>
#include <vector>

#include "brave/components/brave_wallet/browser/brave_wallet_provider_delegate.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

namespace {
constexpr char kTabNotVisibleError[] = "Tab not active";
constexpr char kAccountNotConnectedError[] = "Account not connected";

// APIErrorCode
[[maybe_unused]] constexpr int kAPIErrorInvalidRequest = -1;
constexpr int kAPIErrorInternalError = -2;
constexpr int kAPIErrorRefused = -3;
[[maybe_unused]] constexpr int kAPIErrorAccountChange = -4;

// DataSignErrorCode
[[maybe_unused]] constexpr int kDataSignProofGeneration = 1;
[[maybe_unused]] constexpr int kDataSignAddressNotPK = 2;
[[maybe_unused]] constexpr int kDataSignUserDeclined = 3;

// TxSendErrorCode
[[maybe_unused]] constexpr int kTxSendRefused = 1;
[[maybe_unused]] constexpr int kTxSendFailure = 2;

// TxSignErrorCode
[[maybe_unused]] constexpr int kTxSignProofGeneration = 1;
[[maybe_unused]] constexpr int kTxSignUserDeclined = 2;

mojom::AccountIdPtr FindFirstCardanoHDAccount(KeyringService* keyring_service) {
  auto accounts = keyring_service->GetAllAccountsSync();
  for (const auto& account : accounts->accounts) {
    if (account && account->account_id &&
        IsCardanoMainnetKeyring(account->account_id->keyring_id) &&
        account->account_id->account_index == 0) {
      return account->account_id.Clone();
    }
  }
  return nullptr;
}

}  // namespace

CardanoProviderImpl::CardanoProviderImpl(
    BraveWalletService& brave_wallet_service,
    std::unique_ptr<BraveWalletProviderDelegate> delegate)
    : brave_wallet_service_(brave_wallet_service),
      delegate_(std::move(delegate)) {
  CHECK(delegate_);
  brave_wallet_service_->keyring_service()->AddObserver(
      keyring_observer_receiver_.BindNewPipeAndPassRemote());
}

CardanoProviderImpl::~CardanoProviderImpl() = default;

// mojom::CardanoProvider
void CardanoProviderImpl::IsEnabled(IsEnabledCallback callback) {
  // Mocked values for development usage.
  std::move(callback).Run(true);
}

void CardanoProviderImpl::Enable(EnableCallback callback) {
  if (!delegate_->IsTabVisible()) {
    std::move(callback).Run(mojom::CardanoProviderErrorBundle::New(
        kAPIErrorRefused, kTabNotVisibleError, nullptr));
    return;
  }

  delegate_->WalletInteractionDetected();

  // Mocked values for development usage.
  RequestCardanoPermissions(std::move(callback), delegate_->GetOrigin());
}

mojom::AccountIdPtr CardanoProviderImpl::GetAllowedSelectedAccount() {
  auto cardano_account_id =
      FindFirstCardanoHDAccount(brave_wallet_service_->keyring_service());

  if (!cardano_account_id) {
    return nullptr;
  }

  const auto allowed_accounts = delegate_->GetAllowedAccounts(
      mojom::CoinType::ADA,
      {GetAccountPermissionIdentifier(cardano_account_id)});

  if (!allowed_accounts || allowed_accounts->empty()) {
    return nullptr;
  }

  for (const auto& account :
       brave_wallet_service_->keyring_service()->GetAllAccountInfos()) {
    if (GetAccountPermissionIdentifier(account->account_id) ==
        (*allowed_accounts)[0]) {
      return account->account_id.Clone();
    }
  }
  return nullptr;
}

void CardanoProviderImpl::GetNetworkId(GetNetworkIdCallback callback) {
  auto account_id = GetAllowedSelectedAccount();
  if (!account_id) {
    std::move(callback).Run(
        0, mojom::CardanoProviderErrorBundle::New(
               kAPIErrorRefused, kAccountNotConnectedError, nullptr));
    return;
  }

  delegate_->WalletInteractionDetected();

  // Mocked values for development usage.
  std::move(callback).Run(0, nullptr);
}

void CardanoProviderImpl::GetUsedAddresses(GetUsedAddressesCallback callback) {
  auto account_id = GetAllowedSelectedAccount();
  if (!account_id) {
    std::move(callback).Run(
        {}, mojom::CardanoProviderErrorBundle::New(
                kAPIErrorRefused, kAccountNotConnectedError, nullptr));
    return;
  }

  delegate_->WalletInteractionDetected();

  // Mocked values for development usage.
  std::move(callback).Run({"1", "2", "3"}, nullptr);
}

void CardanoProviderImpl::GetUnusedAddresses(
    GetUnusedAddressesCallback callback) {
  auto account_id = GetAllowedSelectedAccount();
  if (!account_id) {
    std::move(callback).Run(
        {}, mojom::CardanoProviderErrorBundle::New(
                kAPIErrorRefused, kAccountNotConnectedError, nullptr));
    return;
  }

  delegate_->WalletInteractionDetected();

  // Mocked values for development usage.
  std::move(callback).Run({"1", "2", "3"}, nullptr);
}

void CardanoProviderImpl::GetChangeAddress(GetChangeAddressCallback callback) {
  auto account_id = GetAllowedSelectedAccount();
  if (!account_id) {
    std::move(callback).Run(
        {}, mojom::CardanoProviderErrorBundle::New(
                kAPIErrorRefused, kAccountNotConnectedError, nullptr));
    return;
  }

  delegate_->WalletInteractionDetected();

  // Mocked values for development usage.
  std::move(callback).Run("1", nullptr);
}

void CardanoProviderImpl::GetRewardAddresses(
    GetRewardAddressesCallback callback) {
  auto account_id = GetAllowedSelectedAccount();
  if (!account_id) {
    std::move(callback).Run(
        {}, mojom::CardanoProviderErrorBundle::New(
                kAPIErrorRefused, kAccountNotConnectedError, nullptr));
    return;
  }

  delegate_->WalletInteractionDetected();

  // Mocked values for development usage.
  std::move(callback).Run({"2"}, nullptr);
}

void CardanoProviderImpl::GetBalance(GetBalanceCallback callback) {
  auto account_id = GetAllowedSelectedAccount();
  if (!account_id) {
    std::move(callback).Run(
        {}, mojom::CardanoProviderErrorBundle::New(
                kAPIErrorRefused, kAccountNotConnectedError, nullptr));
    return;
  }

  delegate_->WalletInteractionDetected();

  // Mocked values for development usage.
  std::move(callback).Run("2", nullptr);
}

void CardanoProviderImpl::GetUtxos(const std::optional<std::string>& amount,
                                   mojom::CardanoProviderPaginationPtr paginate,
                                   GetUtxosCallback callback) {
  auto account_id = GetAllowedSelectedAccount();
  if (!account_id) {
    std::move(callback).Run(
        {}, mojom::CardanoProviderErrorBundle::New(
                kAPIErrorRefused, kAccountNotConnectedError, nullptr));
    return;
  }

  delegate_->WalletInteractionDetected();

  // Mocked values for development usage.
  std::move(callback).Run(std::vector<std::string>({"1", "2"}), nullptr);
}

void CardanoProviderImpl::SignTx(const std::string& tx_cbor,
                                 bool partial_sign,
                                 SignTxCallback callback) {
  auto account_id = GetAllowedSelectedAccount();
  if (!account_id) {
    std::move(callback).Run(
        {}, mojom::CardanoProviderErrorBundle::New(
                kAPIErrorRefused, kAccountNotConnectedError, nullptr));
    return;
  }

  delegate_->WalletInteractionDetected();

  // Mocked values for development usage.
  std::move(callback).Run("signed", nullptr);
}

void CardanoProviderImpl::SubmitTx(const std::string& signed_tx_cbor,
                                   SubmitTxCallback callback) {
  auto account_id = GetAllowedSelectedAccount();
  if (!account_id) {
    std::move(callback).Run(
        {}, mojom::CardanoProviderErrorBundle::New(
                kAPIErrorRefused, kAccountNotConnectedError, nullptr));
    return;
  }

  delegate_->WalletInteractionDetected();

  // Mocked values for development usage.
  std::move(callback).Run("txhash", nullptr);
}

void CardanoProviderImpl::SignData(const std::string& address,
                                   const std::string& payload_hex,
                                   SignDataCallback callback) {
  auto account_id = GetAllowedSelectedAccount();
  if (!account_id) {
    std::move(callback).Run(
        {}, mojom::CardanoProviderErrorBundle::New(
                kAPIErrorRefused, kAccountNotConnectedError, nullptr));
    return;
  }

  delegate_->WalletInteractionDetected();

  // Mocked values for development usage.
  std::move(callback).Run(mojom::CardanoProviderSignatureResult::New("1", "2"),
                          nullptr);
}

void CardanoProviderImpl::GetCollateral(const std::string& amount,
                                        GetCollateralCallback callback) {
  auto account_id = GetAllowedSelectedAccount();
  if (!account_id) {
    std::move(callback).Run(
        {}, mojom::CardanoProviderErrorBundle::New(
                kAPIErrorRefused, kAccountNotConnectedError, nullptr));
    return;
  }

  delegate_->WalletInteractionDetected();

  // Mocked values for development usage.
  std::move(callback).Run(std::vector<std::string>({"1", "2"}), nullptr);
}

void CardanoProviderImpl::RequestCardanoPermissions(EnableCallback callback,
                                                    const url::Origin& origin) {
  if (delegate_->IsPermissionDenied(mojom::CoinType::ADA)) {
    OnRequestCardanoPermissions(std::move(callback), origin,
                                mojom::RequestPermissionsError::kNone,
                                std::vector<std::string>());
    return;
  }

  // TODO(cypt4): Support multiple Cardano accounts
  auto cardano_account_id =
      FindFirstCardanoHDAccount(brave_wallet_service_->keyring_service());

  if (!cardano_account_id) {
    if (!wallet_onboarding_shown_) {
      delegate_->ShowWalletOnboarding();
      wallet_onboarding_shown_ = true;
    }
    OnRequestCardanoPermissions(std::move(callback), origin,
                                mojom::RequestPermissionsError::kInternal,
                                std::nullopt);
    return;
  }

  if (brave_wallet_service_->keyring_service()->IsLockedSync()) {
    if (pending_request_cardano_permissions_callback_) {
      OnRequestCardanoPermissions(
          std::move(callback), origin,
          mojom::RequestPermissionsError::kRequestInProgress, std::nullopt);
      return;
    }
    pending_request_cardano_permissions_callback_ = std::move(callback);
    pending_request_cardano_permissions_origin_ = origin;

    brave_wallet_service_->keyring_service()->RequestUnlock();
    delegate_->ShowPanel();
    return;
  }

  const auto allowed_accounts = delegate_->GetAllowedAccounts(
      mojom::CoinType::ADA,
      {GetAccountPermissionIdentifier(cardano_account_id)});
  const bool success = allowed_accounts.has_value();

  if (!success) {
    OnRequestCardanoPermissions(std::move(callback), origin,
                                mojom::RequestPermissionsError::kInternal,
                                std::nullopt);
    return;
  }

  if (success && !allowed_accounts->empty()) {
    OnRequestCardanoPermissions(std::move(callback), origin,
                                mojom::RequestPermissionsError::kNone,
                                allowed_accounts);
  } else {
    // Request accounts if no accounts are connected.
    delegate_->RequestPermissions(
        mojom::CoinType::ADA,
        {GetAccountPermissionIdentifier(cardano_account_id)},
        base::BindOnce(&CardanoProviderImpl::OnRequestCardanoPermissions,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback),
                       origin));
  }
}

void CardanoProviderImpl::OnRequestCardanoPermissions(
    EnableCallback callback,
    const url::Origin& origin,
    mojom::RequestPermissionsError error,
    const std::optional<std::vector<std::string>>& allowed_accounts) {
  bool success = error == mojom::RequestPermissionsError::kNone;
  bool has_allowed_account = false;
  if (success && allowed_accounts) {
    has_allowed_account = !allowed_accounts->empty();
  }

  mojom::CardanoProviderErrorBundlePtr error_bundle;
  if (success && !has_allowed_account) {
    error_bundle = mojom::CardanoProviderErrorBundle::New(
        kAPIErrorRefused,
        l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST), nullptr);
  } else if (!success) {
    switch (error) {
      case mojom::RequestPermissionsError::kRequestInProgress:
        error_bundle = mojom::CardanoProviderErrorBundle::New(
            kAPIErrorRefused,
            l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST),
            nullptr);
        break;
      case mojom::RequestPermissionsError::kInternal:
        error_bundle = mojom::CardanoProviderErrorBundle::New(
            kAPIErrorInternalError,
            l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST),
            nullptr);
        break;
      default:
        NOTREACHED() << error;
    }
  }

  std::move(callback).Run(std::move(error_bundle));
}

BraveWalletProviderDelegate* CardanoProviderImpl::delegate() {
  return delegate_.get();
}

void CardanoProviderImpl::Locked() {}

void CardanoProviderImpl::Unlocked() {
  if (pending_request_cardano_permissions_callback_) {
    RequestCardanoPermissions(
        std::move(pending_request_cardano_permissions_callback_.value()),
        std::move(pending_request_cardano_permissions_origin_));
  }
}

void CardanoProviderImpl::SelectedDappAccountChanged(
    mojom::CoinType coin,
    mojom::AccountInfoPtr account) {
  // We have only one possible selected account for Cardano.
}

}  // namespace brave_wallet
