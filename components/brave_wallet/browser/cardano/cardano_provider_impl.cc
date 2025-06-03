/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_provider_impl.h"

#include <utility>

#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "brave/components/brave_wallet/common/brave_wallet_response_helpers.h"

namespace brave_wallet {

namespace {

// Common logic for filtering the list of accounts based on the selected account
std::vector<std::string> FilterAccounts(
    const std::vector<std::string>& accounts,
    const brave_wallet::mojom::AccountInfoPtr& selected_account) {
  // If one of the accounts matches the selected account, then only
  // return that account.  This is for webcompat reasons.
  // Some Dapps select the first account in the list, and some the
  // last. So having only 1 item returned here makes it work for
  // all Dapps.
  std::vector<std::string> filtered_accounts;
  for (const auto& account : accounts) {
    if (selected_account &&
        base::EqualsCaseInsensitiveASCII(account, selected_account->address)) {
      filtered_accounts.clear();
      filtered_accounts.push_back(account);
      break;
    } else {
      filtered_accounts.push_back(account);
    }
  }
  return filtered_accounts;
}

}  // namespace

using RequestPermissionsError = mojom::RequestPermissionsError;

CardanoProviderImpl::CardanoProviderImpl(KeyringService& keyring_service,
                                         std::unique_ptr<BraveWalletProviderDelegateImpl> delegate) :
  keyring_service_(keyring_service), delegate_(std::move(delegate))  {
  keyring_service_->AddObserver(
      keyring_observer_receiver_.BindNewPipeAndPassRemote());
}

CardanoProviderImpl::~CardanoProviderImpl() = default;

// mojom::CardanoProvider
void CardanoProviderImpl::Enable(EnableCallback callback) {
  RequestCardanoPermission(std::move(callback));
  // std::move(callback).Run(true, std::nullopt);
}

mojom::AccountIdPtr CardanoProviderImpl::GetAllowedSelectedAccount() {
  auto account_info =
    keyring_service_->GetHDAccountInfoForKeyring(mojom::KeyringId::kCardanoMainnet, 0u);
  if (!account_info || !account_info->account_id) {
    return nullptr;
  }
  const auto allowed_accounts =
      delegate_->GetAllowedAccounts(mojom::CoinType::ADA, {account_info->account_id->unique_key});
  if (!allowed_accounts || allowed_accounts->empty()) {
    return nullptr;
  }
  for (const auto& account : keyring_service_->GetAllAccountInfos()) {
    if (account->account_id->unique_key == (*allowed_accounts)[0]) {
      return account->account_id.Clone();
    }
  }
  return nullptr;
}

void CardanoProviderImpl::GetNetworkId(GetNetworkIdCallback callback) {
  auto account_id = GetAllowedSelectedAccount();
  if (!account_id) {
    std::move(callback).Run(0, std::nullopt);
    return;
  }
  std::move(callback).Run(0, std::nullopt);
}

void CardanoProviderImpl::GetUsedAddresses(GetUsedAddressesCallback callback) {
  auto account_id = GetAllowedSelectedAccount();
  if (!account_id) {
    std::move(callback).Run({}, std::nullopt);
    return;
  }
  std::move(callback).Run({"1", "2", "3"}, std::nullopt);
}

void CardanoProviderImpl::GetUnusedAddresses(
    GetUnusedAddressesCallback callback) {
  auto account_id = GetAllowedSelectedAccount();
  if (!account_id) {
    std::move(callback).Run({}, std::nullopt);
    return;
  }
  std::move(callback).Run({"1", "2", "3"}, std::nullopt);
}

void CardanoProviderImpl::GetChangeAddress(GetChangeAddressCallback callback) {
  auto account_id = GetAllowedSelectedAccount();
  if (!account_id) {
    std::move(callback).Run({}, std::nullopt);
    return;
  }
  std::move(callback).Run("1", std::nullopt);
}

void CardanoProviderImpl::GetRewardAddresses(
    GetRewardAddressesCallback callback) {
  auto account_id = GetAllowedSelectedAccount();
  if (!account_id) {
    std::move(callback).Run({}, std::nullopt);
    return;
  }
  std::move(callback).Run({"2"}, std::nullopt);
}

void CardanoProviderImpl::GetBalance(GetBalanceCallback callback) {
  auto account_id = GetAllowedSelectedAccount();
  if (!account_id) {
    std::move(callback).Run({}, std::nullopt);
    return;
  }
  std::move(callback).Run("2", std::nullopt);
}

void CardanoProviderImpl::GetUtxos(const std::optional<std::string>& amount,
                                   mojom::CardanoProviderPaginationPtr paginate,
                                   GetUtxosCallback callback) {
  auto account_id = GetAllowedSelectedAccount();
  if (!account_id) {
    std::move(callback).Run({}, std::nullopt);
    return;
  }
  std::move(callback).Run({"1", "2"}, std::nullopt);
}

void CardanoProviderImpl::SignTx(const std::string& tx_cbor,
                                 bool partial_sign,
                                 SignTxCallback callback) {
  auto account_id = GetAllowedSelectedAccount();
  if (!account_id) {
    std::move(callback).Run({}, std::nullopt);
    return;
  }
  std::move(callback).Run("signed", std::nullopt);
}

void CardanoProviderImpl::SubmitTx(const std::string& signed_tx_cbor,
                                   SubmitTxCallback callback) {
  auto account_id = GetAllowedSelectedAccount();
  if (!account_id) {
    std::move(callback).Run({}, std::nullopt);
    return;
  }
  std::move(callback).Run("txhash", std::nullopt);
}

void CardanoProviderImpl::SignData(const std::string& address,
                                   const std::string& payload_hex,
                                   SignDataCallback callback) {
  auto account_id = GetAllowedSelectedAccount();
  if (!account_id) {
    std::move(callback).Run({}, std::nullopt);
    return;
  }
  std::move(callback).Run(mojom::CardanoProviderSignatureResult::New("1", "2"),
                          std::nullopt);
}

void CardanoProviderImpl::SendErrorOnRequest(const mojom::ProviderError& error,
                                             const std::string& error_message,
                                             EnableCallback callback) {
  std::move(callback).Run(false, error_message);
}

void CardanoProviderImpl::RequestCardanoPermission(EnableCallback callback) {
  if (!delegate_->IsTabVisible()) {
    SendErrorOnRequest(
        mojom::ProviderError::kResourceUnavailable,
        "Tab not active",
        std::move(callback));
    return;
  }
  RequestCardanoPermissions(std::move(callback),
                             delegate_->GetOrigin());
  delegate_->WalletInteractionDetected();
}

void CardanoProviderImpl::RequestCardanoPermissions(
    EnableCallback callback,
    const url::Origin& origin) {
  LOG(ERROR) << "XXXZZZ RequestCardanoPermissions";
  DCHECK(delegate_);
  if (delegate_->IsPermissionDenied(mojom::CoinType::ADA)) {
    OnRequestCardanoPermissions(std::move(callback),
                                origin, RequestPermissionsError::kNone,
                                std::vector<std::string>());
    return;
  }

  // TODO(cypt4): Support multiple Cardano accounts
  auto cardano_account =
      keyring_service_->GetHDAccountInfoForKeyring(mojom::KeyringId::kCardanoMainnet, 0);
  if (!cardano_account || !cardano_account->account_id) {
    OnRequestCardanoPermissions(std::move(callback),
                                origin, RequestPermissionsError::kNone,
                                std::vector<std::string>());
    return;
  }

  std::vector<std::string> addresses;
  if (cardano_account) {
    addresses.push_back(cardano_account->account_id->unique_key);
  }

  if (addresses.empty()) {
    if (!wallet_onboarding_shown_) {
      delegate_->ShowWalletOnboarding();
      wallet_onboarding_shown_ = true;
    }
    OnRequestCardanoPermissions(std::move(callback),
                                 origin, RequestPermissionsError::kInternal,
                                 std::nullopt);
    return;
  }

  if (keyring_service_->IsLockedSync()) {
    if (pending_request_cardano_permissions_callback_) {
      OnRequestCardanoPermissions(
          std::move(callback), origin,
          RequestPermissionsError::kRequestInProgress, std::nullopt);
      return;
    }
    pending_request_cardano_permissions_callback_ = std::move(callback);
    pending_request_cardano_permissions_origin_ = origin;

    keyring_service_->RequestUnlock();
    delegate_->ShowPanel();
    return;
  }

  const auto allowed_accounts =
      delegate_->GetAllowedAccounts(mojom::CoinType::ADA, addresses);
  const bool success = allowed_accounts.has_value();

  if (!success) {
    OnRequestCardanoPermissions(std::move(callback),
                                 origin, RequestPermissionsError::kInternal,
                                 std::nullopt);
    return;
  }

  if (success && !allowed_accounts->empty()) {
    OnRequestCardanoPermissions(std::move(callback),
                                 origin, RequestPermissionsError::kNone,
                                 allowed_accounts);
  } else {
    // Request accounts if no accounts are connected.
    delegate_->RequestPermissions(
        mojom::CoinType::ADA, addresses,
        base::BindOnce(&CardanoProviderImpl::OnRequestCardanoPermissions,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback),
                       origin));
  }
}

void CardanoProviderImpl::OnRequestCardanoPermissions(
    EnableCallback callback,
    const url::Origin& origin,
    RequestPermissionsError error,
    const std::optional<std::vector<std::string>>& allowed_accounts) {
  LOG(ERROR) << "XXXZZZ OnRequestCardanoPermissions";
  bool success = error == RequestPermissionsError::kNone;
  std::vector<std::string> accounts;
  if (success && allowed_accounts) {
    accounts = FilterAccounts(
        *allowed_accounts, keyring_service_->GetSelectedCardanoDappAccount());
  }


  std::string first_allowed_account;
  if (accounts.size() > 0) {
    first_allowed_account = base::ToLowerASCII(accounts[0]);
  }

  std::optional<std::string> error_message;
  if (success && accounts.empty()) {
    error_message = l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST);
  } else if (!success) {
    switch (error) {
      case RequestPermissionsError::kRequestInProgress:
        error_message = l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST);
        break;
      case RequestPermissionsError::kInternal:
        error_message = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
        break;
      default:
        NOTREACHED() << error;
    }
  }

  bool reject = !success || accounts.empty();

  std::move(callback).Run(!reject, error_message);
}

void CardanoProviderImpl::Locked() {

}

void CardanoProviderImpl::Unlocked() {
  if (pending_request_cardano_permissions_callback_) {
    RequestCardanoPermissions(
        std::move(pending_request_cardano_permissions_callback_.value()),
        pending_request_cardano_permissions_origin_);
  }
}

void CardanoProviderImpl::SelectedDappAccountChanged(mojom::CoinType coin,
                                mojom::AccountInfoPtr account) {
}

}  // namespace brave_wallet
