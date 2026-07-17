/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_dapp_utils.h"

#include <utility>

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/brave_wallet_provider_delegate.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/common/cardano_address.h"
#include "brave/components/brave_wallet/common/common_utils.h"

namespace brave_wallet {

namespace {

std::string AddressStringOrHex(const std::vector<uint8_t>& address_bytes) {
  if (auto address = CardanoAddress::FromCborBytes(address_bytes)) {
    return address->ToString();
  }
  return base::HexEncodeLower(address_bytes);
}

base::ListValue TokensToValue(const cardano_rpc::Tokens& tokens) {
  base::ListValue result;
  for (const auto& [token_id, value] : tokens) {
    result.Append(base::DictValue()
                      .Set("tokenId", base::HexEncodeLower(token_id))
                      .Set("value", base::NumberToString(value)));
  }
  return result;
}

base::DictValue InputToValue(const CardanoTxDecoder::SerializableTxInput& input,
                             const cardano_rpc::UnspentOutputs& utxos) {
  base::DictValue result;
  result.Set("txHash", base::HexEncodeLower(input.tx_hash));
  result.Set("index", base::NumberToString(input.index));

  auto utxo = FindUtxoByOutpoint(utxos, input);
  result.Set(
      "value",
      utxo ? base::Value(base::NumberToString(utxo->coin_value.lovelace_amount))
           : base::Value());
  result.Set("tokens", utxo
                           ? base::Value(TokensToValue(utxo->coin_value.tokens))
                           : base::Value());
  result.Set("address",
             utxo ? base::Value(utxo->address_to.ToString()) : base::Value());
  return result;
}

base::DictValue OutputToValue(
    const CardanoTxDecoder::SerializableTxOutput& output) {
  return base::DictValue()
      .Set("address", AddressStringOrHex(output.address_bytes))
      .Set("value", base::NumberToString(output.coin_value.lovelace_amount))
      .Set("tokens", TokensToValue(output.coin_value.tokens));
}

}  // namespace

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

base::DictValue FormatCardanoTxDetails(
    const CardanoTxDecoder::SerializableTxBody& tx_body,
    const cardano_rpc::UnspentOutputs& utxos) {
  base::DictValue result;

  if (!tx_body.inputs.empty()) {
    base::ListValue items;
    for (const auto& input : tx_body.inputs) {
      items.Append(InputToValue(input, utxos));
    }
    result.Set("inputs", std::move(items));
  }

  if (!tx_body.outputs.empty()) {
    base::ListValue items;
    for (const auto& output : tx_body.outputs) {
      items.Append(OutputToValue(output));
    }
    result.Set("outputs", std::move(items));
  }

  if (!tx_body.mint.empty()) {
    base::ListValue items;
    for (const auto& token : tx_body.mint) {
      items.Append(base::DictValue()
                       .Set("tokenId", base::HexEncodeLower(token.token_id))
                       .Set("value", base::NumberToString(token.amount)));
    }
    result.Set("mint", std::move(items));
  }

  if (!tx_body.withdrawals.empty()) {
    base::ListValue items;
    for (const auto& withdrawal : tx_body.withdrawals) {
      items.Append(
          base::DictValue()
              .Set("address", AddressStringOrHex(withdrawal.address_bytes))
              .Set("value", base::NumberToString(withdrawal.coin)));
    }
    result.Set("withdrawals", std::move(items));
  }

  if (tx_body.script_data_hash) {
    result.Set("scriptDataHash",
               base::HexEncodeLower(*tx_body.script_data_hash));
  }

  if (!tx_body.collateral.empty()) {
    base::ListValue items;
    for (const auto& input : tx_body.collateral) {
      items.Append(InputToValue(input, utxos));
    }
    result.Set("collateral", std::move(items));
  }

  if (tx_body.collateral_return) {
    result.Set("collateralReturn", OutputToValue(*tx_body.collateral_return));
  }

  if (tx_body.total_collateral) {
    result.Set("totalCollateral",
               base::NumberToString(*tx_body.total_collateral));
  }

  return result;
}

std::vector<std::string> GetCardanoAccountPermissionIdentifiers(
    KeyringService* keyring_service) {
  std::vector<std::string> ids;
  auto accounts = keyring_service->GetAllAccountsSync();
  for (const auto& account : accounts->accounts) {
    if (account && account->account_id &&
        IsCardanoKeyring(account->account_id->keyring_id)) {
      ids.push_back(GetAccountPermissionIdentifier(account->account_id));
    }
  }
  return ids;
}

mojom::AccountIdPtr GetCardanoPreferredDappAccount(
    BraveWalletProviderDelegate* delegate,
    KeyringService* keyring_service) {
  auto cardano_account_ids =
      GetCardanoAccountPermissionIdentifiers(keyring_service);

  if (cardano_account_ids.empty()) {
    return nullptr;
  }

  const auto allowed_accounts =
      delegate->GetAllowedAccounts(mojom::CoinType::ADA, cardano_account_ids);

  if (!allowed_accounts || allowed_accounts->empty()) {
    return nullptr;
  }

  auto selected_account = keyring_service->GetSelectedCardanoDappAccount();
  bool is_selected_account_allowed =
      selected_account &&
      std::ranges::contains(
          *allowed_accounts,
          GetAccountPermissionIdentifier(selected_account->account_id));
  if (is_selected_account_allowed) {
    return selected_account->account_id.Clone();
  }

  // Since there is no account selection when permissions are granted,
  // we use first allowed account.
  // Similar behavior implemented in EthereumProviderImpl.
  for (const auto& account : keyring_service->GetAllAccountInfos()) {
    bool is_account_allowed = std::ranges::contains(
        *allowed_accounts, GetAccountPermissionIdentifier(account->account_id));
    if (is_account_allowed) {
      return account->account_id.Clone();
    }
  }
  return nullptr;
}

mojom::AccountIdPtr GetCardanoPreferredDappAccount(
    KeyringService* keyring_service,
    const std::optional<std::vector<std::string>>& allowed_accounts) {
  if (!allowed_accounts || allowed_accounts->empty()) {
    return nullptr;
  }

  auto cardano_account_ids =
      GetCardanoAccountPermissionIdentifiers(keyring_service);

  if (cardano_account_ids.empty()) {
    return nullptr;
  }

  auto selected_account = keyring_service->GetSelectedCardanoDappAccount();
  bool is_selected_account_allowed =
      selected_account &&
      std::ranges::contains(
          *allowed_accounts,
          GetAccountPermissionIdentifier(selected_account->account_id));
  if (is_selected_account_allowed) {
    return selected_account->account_id.Clone();
  }

  // Since there is no account selection when permissions are granted,
  // we use first allowed account.
  // Similar behavior implemented in EthereumProviderImpl.
  for (const auto& account : keyring_service->GetAllAccountInfos()) {
    bool is_account_allowed = std::ranges::contains(
        *allowed_accounts, GetAccountPermissionIdentifier(account->account_id));
    if (is_account_allowed) {
      return account->account_id.Clone();
    }
  }
  return nullptr;
}

}  // namespace brave_wallet
