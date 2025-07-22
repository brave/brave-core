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
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/cardano_address.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

namespace {
constexpr char kTabNotVisibleError[] = "Tab not active";
constexpr char kUnknownAddressError[] = "Address is unknown";
constexpr char kAccountNotConnectedError[] = "Account not connected";
constexpr char kNotImplemented[] = "Not implemented";

// APIErrorCode
constexpr int kAPIErrorInvalidRequest = -1;
constexpr int kAPIErrorInternalError = -2;
constexpr int kAPIErrorRefused = -3;
[[maybe_unused]] constexpr int kAPIErrorAccountChange = -4;

// DataSignErrorCode
[[maybe_unused]] constexpr int kDataSignProofGeneration = 1;
[[maybe_unused]] constexpr int kDataSignAddressNotPK = 2;
constexpr int kDataSignUserDeclined = 3;

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
  NOTIMPLEMENTED_LOG_ONCE();

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

  std::move(callback).Run(
      IsCardanoMainnetKeyring(account_id->keyring_id) ? 1 : 0, nullptr);
}

void CardanoProviderImpl::GetUsedAddresses(GetUsedAddressesCallback callback) {
  auto account_id = GetAllowedSelectedAccount();
  if (!account_id) {
    std::move(callback).Run(
        std::nullopt,
        mojom::CardanoProviderErrorBundle::New(
            kAPIErrorRefused, kAccountNotConnectedError, nullptr));
    return;
  }

  delegate_->WalletInteractionDetected();

  std::vector<std::string> result;
  for (auto& address :
       brave_wallet_service_->GetCardanoWalletService()->GetUsedAddresses(
           account_id)) {
    auto cardano_address = CardanoAddress::FromString(address->address_string);
    CHECK(cardano_address);

    result.push_back(HexEncodeLower(cardano_address->ToCborBytes()));
  }

  std::move(callback).Run(std::move(result), nullptr);
}

void CardanoProviderImpl::GetUnusedAddresses(
    GetUnusedAddressesCallback callback) {
  auto account_id = GetAllowedSelectedAccount();
  if (!account_id) {
    std::move(callback).Run(
        std::nullopt,
        mojom::CardanoProviderErrorBundle::New(
            kAPIErrorRefused, kAccountNotConnectedError, nullptr));
    return;
  }

  delegate_->WalletInteractionDetected();

  std::vector<std::string> result;
  for (auto& address :
       brave_wallet_service_->GetCardanoWalletService()->GetUnusedAddresses(
           account_id)) {
    auto cardano_address = CardanoAddress::FromString(address->address_string);
    CHECK(cardano_address);

    result.push_back(HexEncodeLower(cardano_address->ToCborBytes()));
  }

  std::move(callback).Run(std::move(result), nullptr);
}

void CardanoProviderImpl::GetChangeAddress(GetChangeAddressCallback callback) {
  auto account_id = GetAllowedSelectedAccount();
  if (!account_id) {
    std::move(callback).Run(
        std::nullopt,
        mojom::CardanoProviderErrorBundle::New(
            kAPIErrorRefused, kAccountNotConnectedError, nullptr));
    return;
  }

  delegate_->WalletInteractionDetected();

  auto address =
      brave_wallet_service_->GetCardanoWalletService()->GetChangeAddress(
          account_id);
  if (!address) {
    std::move(callback).Run(
        std::nullopt,
        mojom::CardanoProviderErrorBundle::New(
            kAPIErrorInternalError, WalletInternalErrorMessage(), nullptr));
    return;
  }

  auto cardano_address = CardanoAddress::FromString(address->address_string);
  CHECK(cardano_address);

  std::move(callback).Run(HexEncodeLower(cardano_address->ToCborBytes()),
                          nullptr);
}

void CardanoProviderImpl::GetRewardAddresses(
    GetRewardAddressesCallback callback) {
  auto account_id = GetAllowedSelectedAccount();
  if (!account_id) {
    std::move(callback).Run(
        std::nullopt,
        mojom::CardanoProviderErrorBundle::New(
            kAPIErrorRefused, kAccountNotConnectedError, nullptr));
    return;
  }

  delegate_->WalletInteractionDetected();

  NOTIMPLEMENTED_LOG_ONCE();
  std::move(callback).Run(
      std::nullopt, mojom::CardanoProviderErrorBundle::New(
                        kAPIErrorInternalError, kNotImplemented, nullptr));
}

void CardanoProviderImpl::GetBalance(GetBalanceCallback callback) {
  auto account_id = GetAllowedSelectedAccount();
  if (!account_id) {
    std::move(callback).Run(
        std::nullopt,
        mojom::CardanoProviderErrorBundle::New(
            kAPIErrorRefused, kAccountNotConnectedError, nullptr));
    return;
  }

  delegate_->WalletInteractionDetected();

  NOTIMPLEMENTED_LOG_ONCE();
  std::move(callback).Run(
      std::nullopt, mojom::CardanoProviderErrorBundle::New(
                        kAPIErrorInternalError, kNotImplemented, nullptr));
}

void CardanoProviderImpl::GetUtxos(const std::optional<std::string>& amount,
                                   mojom::CardanoProviderPaginationPtr paginate,
                                   GetUtxosCallback callback) {
  auto account_id = GetAllowedSelectedAccount();
  if (!account_id) {
    std::move(callback).Run(
        std::nullopt,
        mojom::CardanoProviderErrorBundle::New(
            kAPIErrorRefused, kAccountNotConnectedError, nullptr));
    return;
  }

  delegate_->WalletInteractionDetected();

  NOTIMPLEMENTED_LOG_ONCE();
  std::move(callback).Run(
      std::nullopt, mojom::CardanoProviderErrorBundle::New(
                        kAPIErrorInternalError, kNotImplemented, nullptr));
}

void CardanoProviderImpl::SignTx(const std::string& tx_cbor,
                                 bool partial_sign,
                                 SignTxCallback callback) {
  auto account_id = GetAllowedSelectedAccount();
  if (!account_id) {
    std::move(callback).Run(
        std::nullopt,
        mojom::CardanoProviderErrorBundle::New(
            kAPIErrorRefused, kAccountNotConnectedError, nullptr));
    return;
  }

  delegate_->WalletInteractionDetected();

  NOTIMPLEMENTED_LOG_ONCE();
  std::move(callback).Run(
      std::nullopt, mojom::CardanoProviderErrorBundle::New(
                        kAPIErrorInternalError, kNotImplemented, nullptr));
}

void CardanoProviderImpl::SubmitTx(const std::string& signed_tx_cbor,
                                   SubmitTxCallback callback) {
  auto account_id = GetAllowedSelectedAccount();
  if (!account_id) {
    std::move(callback).Run(
        std::nullopt,
        mojom::CardanoProviderErrorBundle::New(
            kAPIErrorRefused, kAccountNotConnectedError, nullptr));
    return;
  }

  delegate_->WalletInteractionDetected();

  NOTIMPLEMENTED_LOG_ONCE();
  std::move(callback).Run(
      std::nullopt, mojom::CardanoProviderErrorBundle::New(
                        kAPIErrorInternalError, kNotImplemented, nullptr));
}

void CardanoProviderImpl::SignData(const std::string& address,
                                   const std::string& payload_hex,
                                   SignDataCallback callback) {
  auto account_id = GetAllowedSelectedAccount();
  if (!account_id) {
    std::move(callback).Run(
        std::nullopt,
        mojom::CardanoProviderErrorBundle::New(
            kAPIErrorRefused, kAccountNotConnectedError, nullptr));
    return;
  }

  delegate_->WalletInteractionDetected();

  // We now support only one address per cardano account.
  auto supported_signing_address =
      brave_wallet_service_->keyring_service()->GetCardanoAddress(
          account_id,
          mojom::CardanoKeyId::New(mojom::CardanoKeyRole::kExternal, 0));
  if (!supported_signing_address ||
      supported_signing_address->address_string != address) {
    std::move(callback).Run(
        std::nullopt,
        mojom::CardanoProviderErrorBundle::New(kAPIErrorInvalidRequest,
                                               kUnknownAddressError, nullptr));
    return;
  }

  std::vector<uint8_t> message;
  if (!base::HexStringToBytes(payload_hex, &message)) {
    std::move(callback).Run(
        std::nullopt,
        mojom::CardanoProviderErrorBundle::New(
            kAPIErrorInvalidRequest, WalletInternalErrorMessage(), nullptr));
    return;
  }

  auto request = mojom::SignMessageRequest::New(
      MakeOriginInfo(delegate_->GetOrigin()), 0, account_id.Clone(),
      mojom::SignDataUnion::NewCardanoSignData(mojom::CardanoSignData::New(
          std::string(base::as_string_view(message)))),
      mojom::CoinType::ADA,
      brave_wallet_service_->network_manager()->GetCurrentChainId(
          mojom::CoinType::ADA, delegate_->GetOrigin()));

  brave_wallet_service_->AddSignMessageRequest(
      std::move(request),
      base::BindOnce(&CardanoProviderImpl::OnSignMessageRequestProcessed,
                     weak_ptr_factory_.GetWeakPtr(), std::move(account_id),
                     std::move(supported_signing_address->payment_key_id),
                     std::move(message), std::move(callback)));
  delegate_->ShowPanel();
}

void CardanoProviderImpl::OnSignMessageRequestProcessed(
    const mojom::AccountIdPtr& account_id,
    const mojom::CardanoKeyIdPtr& key_id,
    std::vector<uint8_t> message,
    SignDataCallback callback,
    bool approved,
    mojom::EthereumSignatureBytesPtr signature,
    const std::optional<std::string>& error) {
  if (error) {
    std::move(callback).Run(std::nullopt,
                            mojom::CardanoProviderErrorBundle::New(
                                kAPIErrorInternalError, *error, nullptr));
    return;
  }

  if (!approved) {
    std::move(callback).Run(
        std::nullopt,
        mojom::CardanoProviderErrorBundle::New(
            kDataSignUserDeclined,
            l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST),
            nullptr));
    return;
  }

  auto sig_data =
      brave_wallet_service_->keyring_service()
          ->SignCip30MessageByCardanoKeyring(account_id, key_id, message);

  if (!sig_data) {
    std::move(callback).Run(
        std::nullopt,
        mojom::CardanoProviderErrorBundle::New(
            kAPIErrorInternalError, WalletInternalErrorMessage(), nullptr));
    return;
  }

  std::move(callback).Run(std::move(*sig_data), nullptr);
}

void CardanoProviderImpl::GetCollateral(const std::string& amount,
                                        GetCollateralCallback callback) {
  auto account_id = GetAllowedSelectedAccount();
  if (!account_id) {
    std::move(callback).Run(
        std::nullopt,
        mojom::CardanoProviderErrorBundle::New(
            kAPIErrorRefused, kAccountNotConnectedError, nullptr));
    return;
  }

  delegate_->WalletInteractionDetected();

  NOTIMPLEMENTED_LOG_ONCE();
  std::move(callback).Run(
      std::nullopt, mojom::CardanoProviderErrorBundle::New(
                        kAPIErrorInternalError, kNotImplemented, nullptr));
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
