/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_api_impl.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/to_vector.h"
#include "base/functional/callback.h"
#include "base/notimplemented.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/brave_wallet_provider_delegate.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_cip30_serializer.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_dapp_utils.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

namespace {

// APIErrorCode
constexpr int kAPIErrorInvalidRequest = -1;
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
constexpr int kTxSendFailure = 2;

// TxSignErrorCode
[[maybe_unused]] constexpr int kTxSignProofGeneration = 1;
[[maybe_unused]] constexpr int kTxSignUserDeclined = 2;

base::expected<std::optional<std::vector<
                   std::pair<CardanoAddress, cardano_rpc::UnspentOutput>>>,
               mojom::CardanoProviderErrorBundlePtr>
FilterUtxosByAmount(const GetCardanoUtxosTask::UtxoMap& utxo_map,
                    const std::optional<std::string>& amount) {
  std::vector<std::pair<CardanoAddress, cardano_rpc::UnspentOutput>> all_utxos;
  for (const auto& pair_by_address : utxo_map) {
    for (const auto& utxo : pair_by_address.second) {
      all_utxos.push_back({pair_by_address.first, utxo});
    }
  }
  std::sort(all_utxos.begin(), all_utxos.end(),
            [](std::pair<CardanoAddress, cardano_rpc::UnspentOutput>& a,
               std::pair<CardanoAddress, cardano_rpc::UnspentOutput>& b) {
              return (a.second.output_index < b.second.output_index);
            });

  if (amount) {
    std::vector<std::pair<CardanoAddress, cardano_rpc::UnspentOutput>>
        selected_utxos;
    auto numeric_amount = CardanoCip30Serializer::DeserializeAmount(*amount);

    if (!numeric_amount) {
      return base::unexpected(mojom::CardanoProviderErrorBundle::New(
          kAPIErrorInternalError, "Failed to decode amount", nullptr));
    }

    base::CheckedNumeric<uint64_t> accumulated_sum = 0u;

    for (const auto& utxo : all_utxos) {
      accumulated_sum += utxo.second.lovelace_amount;
      if (!accumulated_sum.IsValid()) {
        return base::unexpected(mojom::CardanoProviderErrorBundle::New(
            kAPIErrorInternalError, "Value overflow", nullptr));
      }

      selected_utxos.push_back(utxo);

      if (accumulated_sum.ValueOrDie() >= numeric_amount.value()) {
        break;
      }
    }

    if (accumulated_sum.ValueOrDie() < numeric_amount.value()) {
      return base::ok(std::nullopt);
    }

    return base::ok(selected_utxos);
  }

  return base::ok(all_utxos);
}

base::expected<base::span<const std::string>,
               mojom::CardanoProviderErrorBundlePtr>
ApplyPaginate(base::span<const std::string> serialized_utxos,
              const mojom::CardanoProviderPaginationPtr& paginate) {
  if (!paginate) {
    return serialized_utxos;
  }

  if (paginate->limit <= 0 || paginate->page < 0) {
    return base::unexpected(mojom::CardanoProviderErrorBundle::New(
        kAPIErrorInvalidRequest, "Pagination argument error", nullptr));
  }

  size_t start_pos = 0;
  if (!base::CheckMul<size_t>(paginate->page, paginate->limit)
           .AssignIfValid(&start_pos)) {
    return base::unexpected(mojom::CardanoProviderErrorBundle::New(
        kAPIErrorInternalError, "Numeric error", nullptr));
  }

  if (start_pos >= serialized_utxos.size()) {
    base::CheckedNumeric<int32_t> max_pages_count =
        std::ceil(serialized_utxos.size() / paginate->limit);
    return base::unexpected(mojom::CardanoProviderErrorBundle::New(
        kAPIErrorInvalidRequest, "Pagination error",
        mojom::CardanoProviderPaginationErrorPayload::New(
            max_pages_count.ValueOrDie())));
  }

  auto [_, requested_page] = serialized_utxos.split_at(start_pos);
  if (requested_page.size() > static_cast<uint32_t>(paginate->limit)) {
    requested_page =
        requested_page.first(static_cast<uint32_t>(paginate->limit));
  }

  return requested_page;
}

std::optional<std::pair<CardanoAddress, cardano_rpc::UnspentOutput>>
FindAddressByInput(const GetCardanoUtxosTask::UtxoMap& utxo_map,
                   const CardanoTxDecoder::RestoredTransactionInput& input) {
  for (const auto& by_addr : utxo_map) {
    for (const auto& utxo : by_addr.second) {
      if (utxo.tx_hash == input.tx_hash && utxo.output_index == input.index) {
        return std::pair<CardanoAddress, cardano_rpc::UnspentOutput>(
            by_addr.first, utxo);
      }
    }
  }
  return std::nullopt;
}

bool InsertKnownInputAddresses(
    const GetCardanoUtxosTask::UtxoMap& utxo_map,
    CardanoTxDecoder::RestoredTransaction& transaction,
    bool partial_sign) {
  for (auto& restored_input : transaction.tx_body.inputs) {
    auto find_result = FindAddressByInput(utxo_map, restored_input);

    if (find_result) {
      restored_input.address = std::move(find_result->first);
      restored_input.amount = find_result->second.lovelace_amount;
    } else if (!partial_sign) {
      return false;
    }
  }

  return true;
}

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

  brave_wallet_service_->GetCardanoWalletService()->GetBalance(
      selected_account_.Clone(),
      base::BindOnce(&CardanoApiImpl::OnGetBalance,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void CardanoApiImpl::OnGetBalance(GetBalanceCallback callback,
                                  mojom::CardanoBalancePtr balance,
                                  const std::optional<std::string>& error) {
  if (error) {
    std::move(callback).Run(
        std::nullopt, mojom::CardanoProviderErrorBundle::New(
                          kAPIErrorInternalError, error.value(), nullptr));
    return;
  }

  CHECK(balance);

  std::move(callback).Run(
      CardanoCip30Serializer::SerializeAmount(balance->total_balance), nullptr);
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

  brave_wallet_service_->GetCardanoWalletService()->GetUtxos(
      selected_account_.Clone(),
      base::BindOnce(&CardanoApiImpl::OnGetUtxos,
                     weak_ptr_factory_.GetWeakPtr(), amount,
                     std::move(paginate), std::move(callback)));
}

void CardanoApiImpl::OnGetUtxos(
    const std::optional<std::string>& amount,
    mojom::CardanoProviderPaginationPtr paginate,
    GetUtxosCallback callback,
    base::expected<GetCardanoUtxosTask::UtxoMap, std::string> result) {
  if (!result.has_value()) {
    std::move(callback).Run(
        std::nullopt, mojom::CardanoProviderErrorBundle::New(
                          kAPIErrorInternalError, result.error(), nullptr));
    return;
  }

  auto filter_utxos_result = FilterUtxosByAmount(result.value(), amount);
  if (!filter_utxos_result.has_value()) {
    std::move(callback).Run(std::nullopt,
                            std::move(filter_utxos_result.error()));
    return;
  }

  if (!filter_utxos_result.value()) {
    // Can't achieve amount.
    std::move(callback).Run(std::nullopt, nullptr);
    return;
  }

  auto serialized_utxos =
      CardanoCip30Serializer::SerializeUtxos(**filter_utxos_result);

  auto apply_paginate_result = ApplyPaginate(serialized_utxos, paginate);
  if (!apply_paginate_result.has_value()) {
    std::move(callback).Run(std::nullopt,
                            std::move(apply_paginate_result.error()));
    return;
  }

  std::move(callback).Run(base::ToVector(*apply_paginate_result), nullptr);
}

void CardanoApiImpl::SignTx(const std::string& tx_cbor,
                            bool partial_sign,
                            SignTxCallback callback) {
  auto error = CheckSelectedAccountValid();
  if (error) {
    std::move(callback).Run(std::nullopt, std::move(error));
    return;
  }

  std::vector<uint8_t> tx_cbor_bytes;
  if (!base::HexStringToBytes(tx_cbor, &tx_cbor_bytes)) {
    std::move(callback).Run(
        std::nullopt,
        mojom::CardanoProviderErrorBundle::New(
            kAPIErrorInvalidRequest, "Failed to decode transaction", nullptr));
    return;
  }

  auto restored_tx = CardanoTxDecoder::DecodeTransaction(tx_cbor_bytes);
  if (!restored_tx) {
    std::move(callback).Run(
        std::nullopt,
        mojom::CardanoProviderErrorBundle::New(
            kAPIErrorInternalError, WalletInternalErrorMessage(), nullptr));
    return;
  }

  delegate_->WalletInteractionDetected();

  // Serialized transaction doesn't contain information regarding
  // input address being used, only tx id and input index, so
  // we need to restore utxos addresses fist.
  brave_wallet_service_->GetCardanoWalletService()->GetUtxos(
      selected_account_.Clone(),
      base::BindOnce(&CardanoApiImpl::OnGetUtxosForSignTx,
                     weak_ptr_factory_.GetWeakPtr(), restored_tx.value(),
                     partial_sign, std::move(callback)));
}

void CardanoApiImpl::OnGetUtxosForSignTx(
    CardanoTxDecoder::RestoredTransaction tx,
    bool partial_sign,
    SignTxCallback callback,
    base::expected<GetCardanoUtxosTask::UtxoMap, std::string> utxos) {
  auto account_valid_error = CheckSelectedAccountValid();
  if (account_valid_error) {
    std::move(callback).Run(std::nullopt, std::move(account_valid_error));
    return;
  }

  if (!utxos.has_value()) {
    std::move(callback).Run(
        std::nullopt,
        mojom::CardanoProviderErrorBundle::New(
            kAPIErrorInternalError, WalletInternalErrorMessage(), nullptr));
    return;
  }

  if (!InsertKnownInputAddresses(utxos.value(), tx, partial_sign)) {
    std::move(callback).Run(
        std::nullopt,
        mojom::CardanoProviderErrorBundle::New(
            kAPIErrorInvalidRequest, "Cannot sign all inputs", nullptr));
    return;
  }

  auto request = FromRestoredTransaction(tx);
  if (!request) {
    std::move(callback).Run(
        std::nullopt,
        mojom::CardanoProviderErrorBundle::New(
            kAPIErrorInternalError, WalletInternalErrorMessage(), nullptr));
    return;
  }

  brave_wallet_service_->AddSignCardanoTransactionRequest(
      std::move(request),
      base::BindOnce(&CardanoApiImpl::OnSignTransactionRequestProcessed,
                     weak_ptr_factory_.GetWeakPtr(), tx, std::move(callback)));

  delegate_->ShowPanel();
}

mojom::SignCardanoTransactionRequestPtr CardanoApiImpl::FromRestoredTransaction(
    const CardanoTxDecoder::RestoredTransaction& tx) {
  auto addresses =
      brave_wallet_service_->keyring_service()->GetCardanoAddresses(
          selected_account_);
  if (!addresses) {
    return nullptr;
  }
  auto address_map = GetCardanoAddressesWithKeyIds(*addresses);
  if (!address_map) {
    return nullptr;
  }

  std::vector<mojom::CardanoTxInputPtr> inputs;
  std::vector<mojom::CardanoTxOutputPtr> outputs;

  for (const auto& input : tx.tx_body.inputs) {
    inputs.emplace_back(mojom::CardanoTxInput::New(
        input.address ? input.address->ToString() : "",
        base::HexEncode(input.tx_hash), input.index, input.amount.value_or(0)));
  }

  for (const auto& output : tx.tx_body.outputs) {
    outputs.emplace_back(
        mojom::CardanoTxOutput::New(output.address.ToString(), output.amount));
  }

  return mojom::SignCardanoTransactionRequest::New(
      -1, selected_account_.Clone(), MakeOriginInfo(delegate_->GetOrigin()),
      mojom::ChainId::New(mojom::CoinType::ADA,
                          GetNetworkForCardanoAccount(selected_account_)),
      base::HexEncode(tx.raw_tx_bytes), std::move(inputs), std::move(outputs));
}

void CardanoApiImpl::OnSignTransactionRequestProcessed(
    CardanoTxDecoder::RestoredTransaction tx,
    SignTxCallback callback,
    bool approved,
    const std::optional<std::string>& error) {
  auto account_valid_error = CheckSelectedAccountValid();
  if (account_valid_error) {
    std::move(callback).Run(std::nullopt, std::move(account_valid_error));
    return;
  }

  if (!approved) {
    std::move(callback).Run(std::nullopt,
                            mojom::CardanoProviderErrorBundle::New(
                                kAPIErrorRefused, error.value_or(""), nullptr));
    return;
  }

  auto addresses =
      brave_wallet_service_->keyring_service()->GetCardanoAddresses(
          selected_account_);
  if (!addresses) {
    std::move(callback).Run(
        std::nullopt,
        mojom::CardanoProviderErrorBundle::New(
            kAPIErrorInternalError, WalletInternalErrorMessage(), nullptr));
    return;
  }

  auto address_map = GetCardanoAddressesWithKeyIds(*addresses);
  if (!address_map) {
    std::move(callback).Run(
        std::nullopt,
        mojom::CardanoProviderErrorBundle::New(
            kAPIErrorInternalError, WalletInternalErrorMessage(), nullptr));
    return;
  }

  auto hash = Blake2bHash<kCardanoTxHashSize>({tx.tx_body.raw_body_bytes});

  std::vector<CardanoTxDecoder::CardanoSignMessageResult> sign_results;
  for (const auto& input : tx.tx_body.inputs) {
    if (!input.address) {
      continue;
    }
    auto address_map_item = address_map->find(input.address.value());
    if (address_map_item == address_map->end()) {
      std::move(callback).Run(
          std::nullopt,
          mojom::CardanoProviderErrorBundle::New(
              kAPIErrorInternalError, WalletInternalErrorMessage(), nullptr));
      return;
    }

    auto sign_result =
        brave_wallet_service_->keyring_service()->SignMessageByCardanoKeyring(
            selected_account_, address_map_item->second, hash);

    if (!sign_result) {
      std::move(callback).Run(
          std::nullopt,
          mojom::CardanoProviderErrorBundle::New(
              kAPIErrorInternalError, WalletInternalErrorMessage(), nullptr));
      return;
    }

    sign_results.emplace_back(base::ToVector(sign_result->signature),
                              base::ToVector(sign_result->pubkey));
  }

  auto signed_tx = CardanoTxDecoder::AddWitnessesToTransaction(
      tx.raw_tx_bytes, std::move(sign_results));

  if (!signed_tx) {
    std::move(callback).Run(
        std::nullopt,
        mojom::CardanoProviderErrorBundle::New(
            kAPIErrorInternalError, "Failed to sign transaction", nullptr));
    return;
  }

  std::move(callback).Run(base::HexEncode(signed_tx.value()), nullptr);
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

  auto* cardano_rpc =
      brave_wallet_service_->GetCardanoWalletService()->GetCardanoRpc(
          GetNetworkForCardanoAccount(selected_account_));

  std::vector<uint8_t> message;
  if (!base::HexStringToBytes(signed_tx_cbor, &message)) {
    std::move(callback).Run(
        std::nullopt, mojom::CardanoProviderErrorBundle::New(
                          kTxSendFailure, "Failed to decode CBOR", nullptr));
    return;
  }

  cardano_rpc->PostTransaction(
      std::move(message),
      base::BindOnce(&CardanoApiImpl::OnSubmitTx,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void CardanoApiImpl::OnSubmitTx(SubmitTxCallback callback,
                                base::expected<std::string, std::string> txid) {
  if (txid.has_value()) {
    std::move(callback).Run(txid.value(), nullptr);
  } else {
    std::move(callback).Run(std::nullopt,
                            mojom::CardanoProviderErrorBundle::New(
                                kTxSendFailure, txid.error(), nullptr));
  }
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
