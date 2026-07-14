/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_api_impl.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

#include "base/notimplemented.h"
#include "base/numerics/checked_math.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/brave_wallet_provider_delegate.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_dapp_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_rpc_schema.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/cardano_address.h"
#include "brave/components/brave_wallet/common/common_utils.h"
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

// Pick utxos to reach required coin value. Returns nullopt if it's not
// possible.
std::optional<cardano_rpc::UnspentOutputs> FilterUtxosByAmount(
    cardano_rpc::UnspentOutputs utxos,
    const cardano_rpc::CoinValue& required_coin_value) {
  // Keep track of selected utxos which already part of
  // `accumulated_coin_value`.
  absl::flat_hash_set<const cardano_rpc::UnspentOutput*> selected_utxos;
  cardano_rpc::CoinValue accumulated_coin_value;

  // For each required token iterate over utxos and pick them until required
  // amount is reached.
  for (auto& [required_token_id, required_amount] :
       required_coin_value.tokens) {
    for (auto& utxo : utxos) {
      if (accumulated_coin_value.tokens[required_token_id] >= required_amount) {
        break;
      }

      if (selected_utxos.contains(&utxo)) {
        continue;
      }

      if (!utxo.coin_value.tokens.contains(required_token_id)) {
        continue;
      }

      if (!accumulated_coin_value.Add(utxo.coin_value)) {
        return std::nullopt;
      }
      selected_utxos.insert(&utxo);
    }

    if (accumulated_coin_value.tokens[required_token_id] < required_amount) {
      // Failed to reach required amount of `required_token`.
      return std::nullopt;
    }
  }

  // Iterate over utxos and pick them until required lovelace amount is reached.
  for (auto& utxo : utxos) {
    if (accumulated_coin_value.lovelace_amount >=
        required_coin_value.lovelace_amount) {
      break;
    }

    if (selected_utxos.contains(&utxo)) {
      continue;
    }

    if (!accumulated_coin_value.Add(utxo.coin_value)) {
      return std::nullopt;
    }
    selected_utxos.insert(&utxo);
  }

  if (accumulated_coin_value.lovelace_amount <
      required_coin_value.lovelace_amount) {
    // Failed to reach required amount of lovelace.
    return std::nullopt;
  }

  cardano_rpc::UnspentOutputs result;
  for (auto& utxo : utxos) {
    if (selected_utxos.contains(&utxo)) {
      result.push_back(std::move(utxo));
    }
  }

  return result;
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
    const CardanoTxDecoder::SerializableTxInput& input) {
  for (const auto& utxo : utxos) {
    if (utxo.tx_hash == input.tx_hash && utxo.output_index == input.index) {
      return utxo;
    }
  }
  return std::nullopt;
}

}  // namespace

CardanoApiImpl::CardanoApiImpl(
    BraveWalletService& brave_wallet_service,
    std::unique_ptr<BraveWalletProviderDelegate> delegate,
    mojom::AccountIdPtr selected_account,
    const url::Origin& origin)
    : brave_wallet_service_(brave_wallet_service),
      delegate_(std::move(delegate)),
      selected_account_(std::move(selected_account)),
      origin_(origin) {}

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

  std::move(callback).Run(
      IsCardanoMainnetKeyring(selected_account_->keyring_id) ? 1 : 0, nullptr);
}

void CardanoApiImpl::GetUsedAddresses(GetUsedAddressesCallback callback) {
  auto error = CheckSelectedAccountValid();
  if (error) {
    std::move(callback).Run(std::nullopt, std::move(error));
    return;
  }

  std::vector<std::string> result;
  for (auto& address :
       brave_wallet_service_->GetCardanoWalletService()->GetUsedAddresses(
           selected_account_)) {
    auto cardano_address = CardanoAddress::FromString(address->address_string);
    CHECK(cardano_address);

    result.push_back(base::HexEncodeLower(cardano_address->ToCborBytes()));
  }

  std::move(callback).Run(std::move(result), nullptr);
}

void CardanoApiImpl::GetUnusedAddresses(GetUnusedAddressesCallback callback) {
  auto error = CheckSelectedAccountValid();
  if (error) {
    std::move(callback).Run(std::nullopt, std::move(error));
    return;
  }

  std::vector<std::string> result;
  for (auto& address :
       brave_wallet_service_->GetCardanoWalletService()->GetUnusedAddresses(
           selected_account_)) {
    auto cardano_address = CardanoAddress::FromString(address->address_string);
    CHECK(cardano_address);

    result.push_back(base::HexEncodeLower(cardano_address->ToCborBytes()));
  }

  std::move(callback).Run(std::move(result), nullptr);
}

void CardanoApiImpl::GetChangeAddress(GetChangeAddressCallback callback) {
  auto error = CheckSelectedAccountValid();
  if (error) {
    std::move(callback).Run(std::nullopt, std::move(error));
    return;
  }

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

  std::move(callback).Run(base::HexEncodeLower(cardano_address->ToCborBytes()),
                          nullptr);
}

void CardanoApiImpl::GetRewardAddresses(GetRewardAddressesCallback callback) {
  auto error = CheckSelectedAccountValid();
  if (error) {
    std::move(callback).Run(std::nullopt, std::move(error));
    return;
  }

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

  brave_wallet_service_->GetCardanoWalletService()->GetUtxos(
      selected_account_.Clone(),
      base::BindOnce(&CardanoApiImpl::OnGetUtxosForBalance,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void CardanoApiImpl::OnGetUtxosForBalance(
    GetBalanceCallback callback,
    base::expected<cardano_rpc::UnspentOutputs, std::string> all_utxos) {
  if (!all_utxos.has_value()) {
    std::move(callback).Run(
        std::nullopt, mojom::CardanoProviderErrorBundle::New(
                          kAPIErrorInternalError, all_utxos.error(), nullptr));
    return;
  }

  cardano_rpc::CoinValue sum_utxos;
  for (auto& output : all_utxos.value()) {
    if (!sum_utxos.Add(output.coin_value)) {
      std::move(callback).Run(
          std::nullopt,
          mojom::CardanoProviderErrorBundle::New(
              kAPIErrorInternalError, WalletInternalErrorMessage(), nullptr));
      return;
    }
  }

  auto balance_hex = CardanoTxDecoder::EncodeCoinValue(sum_utxos);
  if (!balance_hex) {
    std::move(callback).Run(
        std::nullopt,
        mojom::CardanoProviderErrorBundle::New(
            kAPIErrorInternalError, WalletInternalErrorMessage(), nullptr));
    return;
  }

  std::move(callback).Run(base::HexEncodeLower(balance_hex.value()), nullptr);
}

void CardanoApiImpl::GetUtxos(const std::optional<std::string>& amount_cbor,
                              mojom::CardanoProviderPaginationPtr paginate,
                              GetUtxosCallback callback) {
  auto error = CheckSelectedAccountValid();
  if (error) {
    std::move(callback).Run(std::nullopt, std::move(error));
    return;
  }

  std::optional<cardano_rpc::CoinValue> required_coin_value;
  if (amount_cbor) {
    std::vector<uint8_t> amount_cbor_bytes;
    if (!base::HexStringToBytes(*amount_cbor, &amount_cbor_bytes)) {
      std::move(callback).Run(
          std::nullopt,
          mojom::CardanoProviderErrorBundle::New(
              kAPIErrorInternalError, WalletParsingErrorMessage(), nullptr));
      return;
    }
    required_coin_value = CardanoTxDecoder::DecodeCoinValue(amount_cbor_bytes);
    if (!required_coin_value) {
      std::move(callback).Run(
          std::nullopt,
          mojom::CardanoProviderErrorBundle::New(
              kAPIErrorInternalError, WalletInternalErrorMessage(), nullptr));
      return;
    }
  }

  brave_wallet_service_->GetCardanoWalletService()->GetUtxos(
      selected_account_.Clone(),
      base::BindOnce(&CardanoApiImpl::OnGetUtxos,
                     weak_ptr_factory_.GetWeakPtr(), required_coin_value,
                     std::move(paginate), std::move(callback)));
}

void CardanoApiImpl::OnGetUtxos(
    std::optional<cardano_rpc::CoinValue> required_coin_value,
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

  cardano_rpc::UnspentOutputs filtered_utxos;

  if (required_coin_value.has_value()) {
    if (auto filtered = FilterUtxosByAmount(std::move(all_utxos.value()),
                                            required_coin_value.value())) {
      filtered_utxos = std::move(*filtered);
    } else {
      std::move(callback).Run(std::nullopt, nullptr);
      return;
    }
  } else {
    filtered_utxos = std::move(all_utxos.value());
  }

  auto paginated_utxos = ApplyPaginate(filtered_utxos, paginate);
  if (!paginated_utxos.has_value()) {
    std::move(callback).Run(std::nullopt, std::move(paginated_utxos.error()));
    return;
  }

  std::vector<std::string> serialized_utxos;
  for (auto& utxo : paginated_utxos.value()) {
    auto serialized_utxo = CardanoTxDecoder::EncodeUtxo(
        CardanoTxDecoder::SerializableTxInput(utxo.tx_hash, utxo.output_index),
        CardanoTxDecoder::SerializableTxOutput(utxo.address_to.ToCborBytes(),
                                               std::move(utxo.coin_value)));
    if (!serialized_utxo) {
      std::move(callback).Run(
          std::nullopt,
          mojom::CardanoProviderErrorBundle::New(
              kAPIErrorInternalError, WalletInternalErrorMessage(), nullptr));
      return;
    }
    serialized_utxos.push_back(base::HexEncodeLower(serialized_utxo.value()));
  }

  std::move(callback).Run(std::move(serialized_utxos), nullptr);
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

  auto decoded_tx = CardanoTxDecoder::DecodeTransaction(tx_cbor_bytes);
  if (!decoded_tx) {
    std::move(callback).Run(
        std::nullopt,
        mojom::CardanoProviderErrorBundle::New(
            kAPIErrorInternalError, WalletInternalErrorMessage(), nullptr));
    return;
  }

  // Serialized transaction doesn't contain information regarding
  // input address being used, only tx id and input index, so
  // we need to restore utxos addresses fist.
  brave_wallet_service_->GetCardanoWalletService()->GetUtxos(
      selected_account_.Clone(),
      base::BindOnce(
          &CardanoApiImpl::OnGetUtxosForSignTx, weak_ptr_factory_.GetWeakPtr(),
          std::move(decoded_tx.value()), partial_sign, std::move(callback)));
}

void CardanoApiImpl::OnGetUtxosForSignTx(
    CardanoTxDecoder::DecodedTx decoded_tx,
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

  auto request = MakeSignCardanoTransactionRequest(decoded_tx, utxos.value());
  if (!request) {
    std::move(callback).Run(
        std::nullopt,
        mojom::CardanoProviderErrorBundle::New(
            kAPIErrorInternalError, WalletInternalErrorMessage(), nullptr));
    return;
  }

  auto payment_key_ids =
      GetPaymentKeyIds(decoded_tx.tx, utxos.value(), partial_sign);
  if (payment_key_ids.empty()) {
    std::move(callback).Run(
        std::nullopt,
        mojom::CardanoProviderErrorBundle::New(
            kAPIErrorInternalError, WalletInternalErrorMessage(), nullptr));
    return;
  }

  brave_wallet_service_->AddSignCardanoTransactionRequest(
      std::move(request),
      base::BindOnce(&CardanoApiImpl::OnSignTransactionRequestProcessed,
                     weak_ptr_factory_.GetWeakPtr(), std::move(decoded_tx),
                     std::move(payment_key_ids), std::move(callback)));

  delegate_->ShowPanel(origin_);
}

mojom::SignCardanoTransactionRequestPtr
CardanoApiImpl::MakeSignCardanoTransactionRequest(
    const CardanoTxDecoder::DecodedTx& decoded_tx,
    const cardano_rpc::UnspentOutputs& utxos) {
  std::vector<mojom::CardanoTxInputPtr> inputs;
  // Fill destination address and value of known inputs.
  for (const auto& input : decoded_tx.tx.tx_body.inputs) {
    auto& inserted = inputs.emplace_back(mojom::CardanoTxInput::New(
        "", base::HexEncode(input.tx_hash), input.index, 0u,
        std::vector<mojom::CardanoTxTokenValuePtr>()));

    if (auto utxo = FindUtxoByOutpoint(utxos, input)) {
      inserted->address = utxo->address_to.ToString();
      inserted->value = utxo->coin_value.lovelace_amount;
      for (auto& token : utxo->coin_value.tokens) {
        inserted->tokens.push_back(mojom::CardanoTxTokenValue::New(
            base::HexEncodeLower(token.first), token.second));
      }
    }
  }

  std::vector<mojom::CardanoTxOutputPtr> outputs;
  for (const auto& output : decoded_tx.tx.tx_body.outputs) {
    auto cardano_address = CardanoAddress::FromCborBytes(output.address_bytes);
    if (!cardano_address) {
      return nullptr;
    }
    outputs.emplace_back(mojom::CardanoTxOutput::New(
        cardano_address->ToString(), output.coin_value.lovelace_amount,
        std::vector<mojom::CardanoTxTokenValuePtr>()));
    for (auto& [token_id, amount] : output.coin_value.tokens) {
      outputs.back()->tokens.push_back(mojom::CardanoTxTokenValue::New(
          base::HexEncodeLower(token_id), amount));
    }
  }

  std::vector<mojom::CardanoTxMintTokenPtr> mint;
  for (const auto& mint_item : decoded_tx.tx.tx_body.mint) {
    mint.push_back(mojom::CardanoTxMintToken::New(
        base::HexEncodeLower(mint_item.token_id), mint_item.amount));
  }

  std::vector<mojom::CardanoTxWithdrawalPtr> withdrawals;
  for (const auto& withdrawal_item : decoded_tx.tx.tx_body.withdrawals) {
    auto cardano_address =
        CardanoAddress::FromCborBytes(withdrawal_item.reward_account);
    if (!cardano_address) {
      return nullptr;
    }

    withdrawals.push_back(mojom::CardanoTxWithdrawal::New(
        cardano_address->ToString(), withdrawal_item.coin));
  }

  return mojom::SignCardanoTransactionRequest::New(
      -1, selected_account_.Clone(), MakeOriginInfo(origin_),
      mojom::ChainId::New(mojom::CoinType::ADA,
                          GetNetworkForCardanoAccount(selected_account_)),
      base::HexEncode(decoded_tx.raw_tx_bytes), std::move(inputs),
      std::move(outputs), std::move(mint), std::move(withdrawals));
}

base::flat_set<mojom::CardanoKeyIdPtr> CardanoApiImpl::GetPaymentKeyIds(
    const CardanoTxDecoder::SerializableTx& tx,
    const cardano_rpc::UnspentOutputs& utxos,
    bool partial_sign) {
  auto addresses =
      brave_wallet_service_->keyring_service()->GetCardanoAddresses(
          selected_account_);
  if (!addresses) {
    return {};
  }
  auto address_map = GetCardanoAddressesWithKeyIds(*addresses);
  if (!address_map) {
    return {};
  }

  base::flat_set<mojom::CardanoKeyIdPtr> payment_key_ids;

  for (const auto& input : tx.tx_body.inputs) {
    if (auto utxo = FindUtxoByOutpoint(utxos, input)) {
      auto payment_key_id = address_map->find(utxo->address_to);
      if (payment_key_id == address_map->end()) {
        return {};
      }
      // Save key id for further signing.
      payment_key_ids.insert(std::move(payment_key_id->second));

    } else if (!partial_sign) {
      // This is full sign, but an unknown utxo so we don't know how to sign it.
      return {};
    }
  }

  return payment_key_ids;
}

void CardanoApiImpl::OnSignTransactionRequestProcessed(
    CardanoTxDecoder::DecodedTx decoded_tx,
    base::flat_set<mojom::CardanoKeyIdPtr> payment_key_ids,
    SignTxCallback callback,
    bool approved,
    const std::optional<std::string>& error) {
  if (auto account_valid_error = CheckSelectedAccountValid()) {
    std::move(callback).Run(std::nullopt, std::move(account_valid_error));
    return;
  }

  if (!approved) {
    std::move(callback).Run(
        std::nullopt, mojom::CardanoProviderErrorBundle::New(
                          kAPIErrorRefused,
                          WalletUserRejectedRequestErrorMessage(), nullptr));
    return;
  }

  auto hash = Blake2bHash<kCardanoTxHashSize>({decoded_tx.raw_body_bytes});

  CardanoTxDecoder::SerializableTxWitness witness;
  for (const auto& payment_key_id : payment_key_ids) {
    if (!payment_key_id) {
      continue;
    }

    auto sign_result =
        brave_wallet_service_->keyring_service()->SignMessageByCardanoKeyring(
            selected_account_, payment_key_id, hash);

    if (!sign_result) {
      std::move(callback).Run(
          std::nullopt,
          mojom::CardanoProviderErrorBundle::New(
              kAPIErrorInternalError, WalletInternalErrorMessage(), nullptr));
      return;
    }

    // If signing pubkey already appears in the witness set - skip it as we
    // don't need to add it again.
    if (std::ranges::contains(
            decoded_tx.tx.tx_witness.vkey_witness_set, sign_result->pubkey,
            &CardanoTxDecoder::SerializableVkeyWitness::public_key)) {
      continue;
    }

    auto& vkey_witness = witness.vkey_witness_set.emplace_back();
    vkey_witness.signature_bytes = sign_result->signature,
    vkey_witness.public_key = sign_result->pubkey;
  }

  auto witness_bytes = CardanoTxDecoder::EncodeWitness(witness);

  if (!witness_bytes) {
    std::move(callback).Run(
        std::nullopt,
        mojom::CardanoProviderErrorBundle::New(
            kAPIErrorInternalError, "Failed to sign transaction", nullptr));
    return;
  }

  std::move(callback).Run(base::HexEncode(witness_bytes.value()), nullptr);
}

void CardanoApiImpl::SignData(const std::string& address,
                              const std::string& payload_hex,
                              SignDataCallback callback) {
  auto error = CheckSelectedAccountValid();
  if (error) {
    std::move(callback).Run(std::nullopt, std::move(error));
    return;
  }

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
      MakeOriginInfo(origin_), 0, selected_account_.Clone(),
      mojom::SignDataUnion::NewCardanoSignData(mojom::CardanoSignData::New(
          std::string(base::as_string_view(message)))),
      mojom::CoinType::ADA,
      brave_wallet_service_->network_manager()->GetCurrentChainId(
          mojom::CoinType::ADA, origin_));

  brave_wallet_service_->AddSignMessageRequest(
      std::move(request),
      base::BindOnce(&CardanoApiImpl::OnSignMessageRequestProcessed,
                     weak_ptr_factory_.GetWeakPtr(), selected_account_.Clone(),
                     std::move(supported_signing_address->payment_key_id),
                     std::move(message), std::move(callback)));
  delegate_->ShowPanel(origin_);
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
