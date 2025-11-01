/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_api_impl.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/to_vector.h"
#include "base/functional/callback.h"
#include "base/notimplemented.h"
#include "base/numerics/checked_math.h"
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

// Check if sum `utxos` amounts don't overflow.
bool ValidateUtxosAmountsSum(
    base::span<const cardano_rpc::UnspentOutput> utxos) {
  base::CheckedNumeric<uint64_t> accumulated_sum = 0u;

  for (auto& utxo : utxos) {
    accumulated_sum += utxo.lovelace_amount;
  }

  return accumulated_sum.IsValid();
}

// Return all utxos if there is no amount limit. Otherwise pick one by one
// until amount is reached.
std::optional<cardano_rpc::UnspentOutputs> FilterUtxosByAmount(
    const cardano_rpc::UnspentOutputs& utxos,
    const std::optional<uint64_t>& amount) {
  if (!amount) {
    return utxos;
  }

  cardano_rpc::UnspentOutputs selected_utxos;

  base::CheckedNumeric<uint64_t> accumulated_sum = 0u;
  for (auto& utxo : utxos) {
    accumulated_sum += utxo.lovelace_amount;

    selected_utxos.push_back(std::move(utxo));

    // Utxos already have been validated not to overflow.
    if (accumulated_sum.ValueOrDie() >= amount.value()) {
      return selected_utxos;
    }
  }

  // Utxos sum did not reach amount.
  return std::nullopt;
}

base::expected<base::span<const cardano_rpc::UnspentOutput>,
               mojom::CardanoProviderErrorBundlePtr>
ApplyPaginate(base::span<const cardano_rpc::UnspentOutput> utxos,
              const mojom::CardanoProviderPaginationPtr& paginate) {
  if (!paginate) {
    return utxos;
  }

  uint32_t limit = 0;
  uint32_t page = 0;

  if (!base::CheckedNumeric(paginate->limit).AssignIfValid(&limit) ||
      !base::CheckedNumeric(paginate->page).AssignIfValid(&page) ||
      limit == 0) {
    return base::unexpected(mojom::CardanoProviderErrorBundle::New(
        kAPIErrorInvalidRequest, "Pagination argument error", nullptr));
  }

  size_t start_pos = 0;
  if (!base::CheckMul<size_t>(page, limit).AssignIfValid(&start_pos)) {
    return base::unexpected(mojom::CardanoProviderErrorBundle::New(
        kAPIErrorInternalError, "Numeric error", nullptr));
  }

  if (start_pos >= utxos.size()) {
    base::CheckedNumeric<int32_t> max_pages_count = utxos.size() / limit;
    if (utxos.size() % limit != 0) {
      max_pages_count++;
    }
    return base::unexpected(mojom::CardanoProviderErrorBundle::New(
        kAPIErrorInvalidRequest, "Pagination error",
        mojom::CardanoProviderPaginationErrorPayload::New(
            max_pages_count.ValueOrDefault(0))));
  }

  auto [_, requested_page] = utxos.split_at(start_pos);
  if (requested_page.size() > limit) {
    requested_page = requested_page.first(limit);
  }

  return requested_page;
}

std::optional<cardano_rpc::UnspentOutput> FindUtxoByOutpoint(
    const cardano_rpc::UnspentOutputs& utxos,
    const CardanoTxDecoder::RestoredTransactionInput& input) {
  for (const auto& utxo : utxos) {
    if (utxo.tx_hash == input.tx_hash && utxo.output_index == input.index) {
      return utxo;
    }
  }
  return std::nullopt;
}

bool InsertKnownInputAddresses(
    const cardano_rpc::UnspentOutputs& utxos,
    CardanoTxDecoder::RestoredTransaction& transaction,
    bool partial_sign) {
  for (auto& restored_input : transaction.tx_body.inputs) {
    auto utxo = FindUtxoByOutpoint(utxos, restored_input);

    if (utxo) {
      restored_input.address = utxo->address_to;
      restored_input.amount = utxo->lovelace_amount;
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

  std::vector<std::string> result;
  if (auto address =
          brave_wallet_service_->GetCardanoWalletService()->GetStakeAddress(
              selected_account_)) {
    result.push_back(HexEncodeLower(address->ToCborBytes()));
  }

  std::move(callback).Run(std::move(result), nullptr);
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

  auto amount_hex =
      CardanoCip30Serializer::SerializeAmount(balance->total_balance);
  if (!amount_hex) {
    std::move(callback).Run(
        std::nullopt,
        mojom::CardanoProviderErrorBundle::New(
            kAPIErrorInternalError, WalletInternalErrorMessage(), nullptr));
    return;
  }

  std::move(callback).Run(*amount_hex, nullptr);
}

void CardanoApiImpl::GetUtxos(const std::optional<std::string>& amount_cbor,
                              mojom::CardanoProviderPaginationPtr paginate,
                              GetUtxosCallback callback) {
  auto error = CheckSelectedAccountValid();
  if (error) {
    std::move(callback).Run(std::nullopt, std::move(error));
    return;
  }

  delegate_->WalletInteractionDetected();

  std::optional<uint64_t> amount;
  if (amount_cbor) {
    amount = CardanoCip30Serializer::DeserializeAmount(*amount_cbor);
    if (!amount) {
      std::move(callback).Run(
          std::nullopt,
          mojom::CardanoProviderErrorBundle::New(
              kAPIErrorInternalError, WalletParsingErrorMessage(), nullptr));
      return;
    }
  }

  brave_wallet_service_->GetCardanoWalletService()->GetUtxos(
      selected_account_.Clone(),
      base::BindOnce(&CardanoApiImpl::OnGetUtxos,
                     weak_ptr_factory_.GetWeakPtr(), amount,
                     std::move(paginate), std::move(callback)));
}

void CardanoApiImpl::OnGetUtxos(
    std::optional<uint64_t> amount,
    mojom::CardanoProviderPaginationPtr paginate,
    GetUtxosCallback callback,
    base::expected<cardano_rpc::UnspentOutputs, std::string> all_utxos) {
  if (!all_utxos.has_value()) {
    std::move(callback).Run(
        std::nullopt, mojom::CardanoProviderErrorBundle::New(
                          kAPIErrorInternalError, all_utxos.error(), nullptr));
    return;
  }

  // Need some stable order for same collection of utxos in case of paging.
  std::ranges::sort(all_utxos.value(), [](cardano_rpc::UnspentOutput& a,
                                          cardano_rpc::UnspentOutput& b) {
    return std::tie(a.tx_hash, a.output_index) <
           std::tie(b.tx_hash, b.output_index);
  });

  if (!ValidateUtxosAmountsSum(all_utxos.value())) {
    std::move(callback).Run(
        std::nullopt,
        mojom::CardanoProviderErrorBundle::New(
            kAPIErrorInternalError, WalletInternalErrorMessage(), nullptr));
    return;
  }

  auto filtered_utxos = FilterUtxosByAmount(all_utxos.value(), amount);
  if (!filtered_utxos) {
    // Can't reach amount.
    std::move(callback).Run(std::nullopt, nullptr);
    return;
  }

  auto paginated_utxos = ApplyPaginate(filtered_utxos.value(), paginate);
  if (!paginated_utxos.has_value()) {
    std::move(callback).Run(std::nullopt, std::move(paginated_utxos.error()));
    return;
  }

  auto serialized_utxos =
      CardanoCip30Serializer::SerializeUtxos(paginated_utxos.value());
  if (!serialized_utxos.has_value()) {
    std::move(callback).Run(
        std::nullopt,
        mojom::CardanoProviderErrorBundle::New(
            kAPIErrorInternalError, WalletInternalErrorMessage(), nullptr));
    return;
  }

  std::move(callback).Run(*serialized_utxos, nullptr);
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
            kAPIErrorInvalidRequest, WalletParsingErrorMessage(), nullptr));
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
    base::expected<cardano_rpc::UnspentOutputs, std::string> utxos) {
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
  auto account_id = GetCardanoPreferredDappAccount(
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
