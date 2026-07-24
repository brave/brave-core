/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_DAPP_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_DAPP_UTILS_H_

#include <optional>
#include <string>
#include <vector>

#include "base/values.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_rpc_schema.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_tx_decoder.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

class BraveWalletProviderDelegate;
class KeyringService;

std::vector<std::string> GetCardanoAccountPermissionIdentifiers(
    KeyringService* keyring_service);

mojom::AccountIdPtr GetCardanoPreferredDappAccount(
    BraveWalletProviderDelegate* delegate,
    KeyringService* keyring_service);

mojom::AccountIdPtr GetCardanoPreferredDappAccount(
    KeyringService* keyring_service,
    const std::optional<std::vector<std::string>>&
        allowed_accounts_from_request);

std::optional<cardano_rpc::UnspentOutput> FindUtxoByOutpoint(
    const cardano_rpc::UnspentOutputs& utxos,
    const CardanoTxDecoder::SerializableTxInput& input);

// Builds a JSON-serializable representation of `tx_body` (inputs, outputs,
// mint, withdrawals, collateral, etc.) resolving input/collateral values and
// addresses against `utxos`. Used to populate the human-readable `details`
// field shown in the Cardano transaction signing UI.
base::DictValue FormatCardanoTxDetails(
    const CardanoTxDecoder::SerializableTxBody& tx_body,
    const cardano_rpc::UnspentOutputs& utxos);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_DAPP_UTILS_H_
