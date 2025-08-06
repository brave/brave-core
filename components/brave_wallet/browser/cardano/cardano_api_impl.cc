/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_api_impl.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/notimplemented.h"
#include "brave/components/brave_wallet/browser/brave_wallet_provider_delegate.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_dapp_utils.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

namespace {

// APIErrorCode
[[maybe_unused]] constexpr int kAPIErrorInvalidRequest = -1;
[[maybe_unused]] constexpr int kAPIErrorInternalError = -2;
constexpr int kAPIErrorRefused = -3;
[[maybe_unused]] constexpr int kAPIErrorAccountChange = -4;

constexpr char kUnknownAddressError[] = "Address is unknown";
constexpr char kAccountNotConnectedError[] = "Account not connected";
constexpr char kAccountChangedError[] = "Account has been changed";
constexpr char kNotImplemented[] = "Not implemented";

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

}  // namespace

CardanoApiImpl::CardanoApiImpl(
    BraveWalletService& brave_wallet_service,
    std::unique_ptr<BraveWalletProviderDelegate> delegate,
    mojom::AccountIdPtr selected_account)
    : brave_wallet_service_(brave_wallet_service),
      delegate_(std::move(delegate)),
      selected_account_(std::move(selected_account)) {}

CardanoApiImpl::~CardanoApiImpl() = default;

BraveWalletProviderDelegate* CardanoApiImpl::delegate() {
  return delegate_.get();
}

void CardanoApiImpl::GetNetworkId(GetNetworkIdCallback callback) {
  auto error = CheckSelectedAccountValid();
  if (error) {
    std::move(callback).Run(false, std::move(error));
    return;
  }

  delegate_->WalletInteractionDetected();

  std::move(callback).Run(
      IsCardanoMainnetKeyring(selected_account_->keyring_id) ? 1 : 0, nullptr);
}

void CardanoApiImpl::GetUsedAddresses(GetUsedAddressesCallback callback) {
  auto error = CheckSelectedAccountValid();
  if (error) {
    std::move(callback).Run(std::nullopt, std::move(error));
    return;
  }

  delegate_->WalletInteractionDetected();

  std::vector<std::string> result;
  for (auto& address :
       brave_wallet_service_->GetCardanoWalletService()->GetUsedAddresses(
           selected_account_)) {
    auto cardano_address = CardanoAddress::FromString(address->address_string);
    CHECK(cardano_address);

    result.push_back(HexEncodeLower(cardano_address->ToCborBytes()));
  }

  std::move(callback).Run(std::move(result), nullptr);
}

void CardanoApiImpl::GetUnusedAddresses(GetUnusedAddressesCallback callback) {
  auto error = CheckSelectedAccountValid();
  if (error) {
    std::move(callback).Run(std::nullopt, std::move(error));
    return;
  }

  delegate_->WalletInteractionDetected();

  std::vector<std::string> result;
  for (auto& address :
       brave_wallet_service_->GetCardanoWalletService()->GetUnusedAddresses(
           selected_account_)) {
    auto cardano_address = CardanoAddress::FromString(address->address_string);
    CHECK(cardano_address);

    result.push_back(HexEncodeLower(cardano_address->ToCborBytes()));
  }

  std::move(callback).Run(std::move(result), nullptr);
}

void CardanoApiImpl::GetChangeAddress(GetChangeAddressCallback callback) {
  auto error = CheckSelectedAccountValid();
  if (error) {
    std::move(callback).Run(std::nullopt, std::move(error));
    return;
  }

  delegate_->WalletInteractionDetected();

  auto address =
      brave_wallet_service_->GetCardanoWalletService()->GetChangeAddress(
          selected_account_);
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

void CardanoApiImpl::GetRewardAddresses(GetRewardAddressesCallback callback) {
  auto error = CheckSelectedAccountValid();
  if (error) {
    std::move(callback).Run(std::nullopt, std::move(error));
    return;
  }

  delegate_->WalletInteractionDetected();

  NOTIMPLEMENTED_LOG_ONCE();
  std::move(callback).Run(
      std::nullopt, mojom::CardanoProviderErrorBundle::New(
                        kAPIErrorInternalError, kNotImplemented, nullptr));
}

void CardanoApiImpl::GetBalance(GetBalanceCallback callback) {
  auto error = CheckSelectedAccountValid();
  if (error) {
    std::move(callback).Run(std::nullopt, std::move(error));
    return;
  }

  delegate_->WalletInteractionDetected();

  NOTIMPLEMENTED_LOG_ONCE();
  std::move(callback).Run(
      std::nullopt, mojom::CardanoProviderErrorBundle::New(
                        kAPIErrorInternalError, kNotImplemented, nullptr));
}

void CardanoApiImpl::GetUtxos(const std::optional<std::string>& amount,
                              mojom::CardanoProviderPaginationPtr paginate,
                              GetUtxosCallback callback) {
  auto error = CheckSelectedAccountValid();
  if (error) {
    std::move(callback).Run(std::nullopt, std::move(error));
    return;
  }
  delegate_->WalletInteractionDetected();

  NOTIMPLEMENTED_LOG_ONCE();
  std::move(callback).Run(
      std::nullopt, mojom::CardanoProviderErrorBundle::New(
                        kAPIErrorInternalError, kNotImplemented, nullptr));
}

void CardanoApiImpl::SignTx(const std::string& tx_cbor,
                            bool partial_sign,
                            SignTxCallback callback) {
  auto error = CheckSelectedAccountValid();
  if (error) {
    std::move(callback).Run(std::nullopt, std::move(error));
    return;
  }

  delegate_->WalletInteractionDetected();

  NOTIMPLEMENTED_LOG_ONCE();
  std::move(callback).Run(
      std::nullopt, mojom::CardanoProviderErrorBundle::New(
                        kAPIErrorInternalError, kNotImplemented, nullptr));
}
void CardanoApiImpl::SignData(const std::string& address,
                              const std::string& payload_hex,
                              SignDataCallback callback) {
  auto error = CheckSelectedAccountValid();
  if (error) {
    std::move(callback).Run(std::nullopt, std::move(error));
    return;
  }

  delegate_->WalletInteractionDetected();

  // We now support only one address per cardano account.
  auto supported_signing_address =
      brave_wallet_service_->keyring_service()->GetCardanoAddress(
          selected_account_,
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
      MakeOriginInfo(delegate_->GetOrigin()), 0, selected_account_.Clone(),
      mojom::SignDataUnion::NewCardanoSignData(mojom::CardanoSignData::New(
          std::string(base::as_string_view(message)))),
      mojom::CoinType::ADA,
      brave_wallet_service_->network_manager()->GetCurrentChainId(
          mojom::CoinType::ADA, delegate_->GetOrigin()));

  brave_wallet_service_->AddSignMessageRequest(
      std::move(request),
      base::BindOnce(&CardanoApiImpl::OnSignMessageRequestProcessed,
                     weak_ptr_factory_.GetWeakPtr(), selected_account_.Clone(),
                     std::move(supported_signing_address->payment_key_id),
                     std::move(message), std::move(callback)));
  delegate_->ShowPanel();
}

void CardanoApiImpl::OnSignMessageRequestProcessed(
    const mojom::AccountIdPtr& account_id,
    const mojom::CardanoKeyIdPtr& key_id,
    const std::vector<uint8_t>& message,
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

  auto sig_data = brave_wallet_service_->keyring_service()
                      ->SignCip30MessageByCardanoKeyring(
                          selected_account_.Clone(), key_id, message);

  if (!sig_data) {
    std::move(callback).Run(
        std::nullopt,
        mojom::CardanoProviderErrorBundle::New(
            kAPIErrorInternalError, WalletInternalErrorMessage(), nullptr));
    return;
  }

  std::move(callback).Run(std::move(*sig_data), nullptr);
}

void CardanoApiImpl::SubmitTx(const std::string& signed_tx_cbor,
                              SubmitTxCallback callback) {
  auto error = CheckSelectedAccountValid();
  if (error) {
    std::move(callback).Run(std::nullopt, std::move(error));
    return;
  }

  delegate_->WalletInteractionDetected();

  NOTIMPLEMENTED_LOG_ONCE();
  std::move(callback).Run(
      std::nullopt, mojom::CardanoProviderErrorBundle::New(
                        kAPIErrorInternalError, kNotImplemented, nullptr));
}

void CardanoApiImpl::GetCollateral(const std::string& amount,
                                   GetCollateralCallback callback) {
  auto error = CheckSelectedAccountValid();
  if (error) {
    std::move(callback).Run(std::nullopt, std::move(error));
    return;
  }

  delegate_->WalletInteractionDetected();

  NOTIMPLEMENTED_LOG_ONCE();
  std::move(callback).Run(
      std::nullopt, mojom::CardanoProviderErrorBundle::New(
                        kAPIErrorInternalError, kNotImplemented, nullptr));
}

mojom::CardanoProviderErrorBundlePtr
CardanoApiImpl::CheckSelectedAccountValid() {
  auto account_id = GetCardanoPereferedDappAccount(
      delegate(), brave_wallet_service_->keyring_service());
  if (!account_id) {
    return mojom::CardanoProviderErrorBundle::New(
        kAPIErrorRefused, kAccountNotConnectedError, nullptr);
  }

  if (account_id->unique_key != selected_account_->unique_key) {
    return mojom::CardanoProviderErrorBundle::New(
        kAPIErrorAccountChange, kAccountChangedError, nullptr);
  }
  return nullptr;
}

}  // namespace brave_wallet
