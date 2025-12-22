/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_wallet_service.h"

#include <stdint.h>

#include <map>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/notreached.h"
#include "base/strings/string_number_conversions.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_create_transaction_task.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_get_utxos_task.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction_serializer.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/cardano_address.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_wallet {

namespace {

mojom::CardanoBalancePtr BalanceFromUtxos(
    const cardano_rpc::UnspentOutputs& utxos) {
  base::CheckedNumeric<uint64_t> total_balance = 0;
  for (const auto& utxo : utxos) {
    total_balance += utxo.lovelace_amount;
  }

  auto result = mojom::CardanoBalance::New();
  if (!total_balance.AssignIfValid(&result->total_balance)) {
    return nullptr;
  }

  return result;
}

}  // namespace

CardanoWalletService::CardanoWalletService(
    KeyringService& keyring_service,
    NetworkManager& network_manager,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : keyring_service_(keyring_service),
      network_manager_(network_manager),
      cardano_mainnet_rpc_(mojom::kCardanoMainnet,
                           network_manager,
                           url_loader_factory),
      cardano_testnet_rpc_(mojom::kCardanoTestnet,
                           network_manager,
                           url_loader_factory) {}

CardanoWalletService::~CardanoWalletService() = default;

void CardanoWalletService::Bind(
    mojo::PendingReceiver<mojom::CardanoWalletService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void CardanoWalletService::Reset() {
  weak_ptr_factory_.InvalidateWeakPtrs();
}

void CardanoWalletService::GetBalance(mojom::AccountIdPtr account_id,
                                      GetBalanceCallback callback) {
  GetUtxos(account_id.Clone(),
           base::BindOnce(&CardanoWalletService::OnGetUtxosForGetBalance,
                          weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void CardanoWalletService::OnGetUtxosForGetBalance(
    GetBalanceCallback callback,
    base::expected<cardano_rpc::UnspentOutputs, std::string> utxos) {
  if (!utxos.has_value()) {
    std::move(callback).Run(nullptr, utxos.error());
    return;
  }
  auto balance = BalanceFromUtxos(utxos.value());
  if (!balance) {
    std::move(callback).Run(nullptr, WalletInternalErrorMessage());
    return;
  }
  std::move(callback).Run(std::move(balance), std::nullopt);
}

void CardanoWalletService::DiscoverNextUnusedAddress(
    const mojom::AccountIdPtr& account_id,
    mojom::CardanoKeyRole role,
    DiscoverNextUnusedAddressCallback callback) {
  CHECK(IsCardanoAccount(account_id));

  // TODO(https://github.com/brave/brave-browser/issues/45278): this always
  // returns first address.
  auto address = keyring_service().GetCardanoAddress(
      account_id, mojom::CardanoKeyId::New(role, 0));
  if (!address) {
    std::move(callback).Run(base::unexpected(WalletInternalErrorMessage()));
    return;
  }

  std::move(callback).Run(std::move(address));
}

void CardanoWalletService::GetUtxos(mojom::AccountIdPtr account_id,
                                    GetUtxosCallback callback) {
  auto addresses = keyring_service().GetCardanoAddresses(account_id);
  if (!addresses) {
    std::move(callback).Run(base::unexpected(WalletInternalErrorMessage()));
    return;
  }

  std::vector<CardanoAddress> cardano_addresses;
  for (const auto& address : *addresses) {
    if (auto cardano_address =
            CardanoAddress::FromString(address->address_string)) {
      cardano_addresses.push_back(std::move(*cardano_address));
    }
  }

  auto [task_it, inserted] =
      get_cardano_utxo_tasks_.insert(std::make_unique<GetCardanoUtxosTask>(
          *this, GetNetworkForCardanoAccount(account_id),
          std::move(cardano_addresses)));
  CHECK(inserted);
  auto* task_ptr = task_it->get();

  task_ptr->Start(base::BindOnce(&CardanoWalletService::OnGetUtxosTaskDone,
                                 weak_ptr_factory_.GetWeakPtr(), task_ptr,
                                 std::move(callback)));
}

void CardanoWalletService::OnGetUtxosTaskDone(
    GetCardanoUtxosTask* task,
    GetUtxosCallback callback,
    base::expected<cardano_rpc::UnspentOutputs, std::string> result) {
  get_cardano_utxo_tasks_.erase(task);

  std::move(callback).Run(std::move(result));
}

void CardanoWalletService::CreateCardanoTransaction(
    mojom::AccountIdPtr account_id,
    const CardanoAddress& address_to,
    uint64_t amount,
    bool sending_max_amount,
    CardanoCreateTransactionTaskCallback callback) {
  CHECK(IsCardanoAccount(account_id));

  auto [task_it, inserted] = create_transaction_tasks_.insert(
      std::make_unique<CardanoCreateTransactionTask>(
          *this, account_id, address_to, amount, sending_max_amount));
  CHECK(inserted);
  auto* task_ptr = task_it->get();

  task_ptr->Start(base::BindOnce(
      &CardanoWalletService::OnCreateCardanoTransactionTaskDone,
      weak_ptr_factory_.GetWeakPtr(), task_ptr, std::move(callback)));
}

void CardanoWalletService::OnCreateCardanoTransactionTaskDone(
    CardanoCreateTransactionTask* task,
    CardanoCreateTransactionTaskCallback callback,
    base::expected<CardanoTransaction, std::string> result) {
  create_transaction_tasks_.erase(task);

  std::move(callback).Run(std::move(result));
}

void CardanoWalletService::SignAndPostTransaction(
    const mojom::AccountIdPtr& account_id,
    CardanoTransaction cardano_transaction,
    SignAndPostTransactionCallback callback) {
  CHECK(IsCardanoAccount(account_id));

  if (!SignTransactionInternal(cardano_transaction, account_id)) {
    std::move(callback).Run("", std::move(cardano_transaction),
                            WalletInternalErrorMessage());
    return;
  }

  CHECK(cardano_transaction.IsSigned());
  auto serialized_transaction =
      CardanoTransactionSerializer().SerializeTransaction(cardano_transaction);

  GetCardanoRpc(GetNetworkForCardanoAccount(account_id))
      ->PostTransaction(
          serialized_transaction,
          base::BindOnce(&CardanoWalletService::OnPostTransaction,
                         weak_ptr_factory_.GetWeakPtr(),
                         std::move(cardano_transaction), std::move(callback)));
}

bool CardanoWalletService::SignTransactionInternal(
    CardanoTransaction& tx,
    const mojom::AccountIdPtr& account_id) {
  auto addresses = keyring_service_->GetCardanoAddresses(account_id);
  if (!addresses) {
    return false;
  }
  auto address_map = GetCardanoAddressesWithKeyIds(*addresses);
  if (!address_map) {
    return false;
  }

  auto hash = CardanoTransactionSerializer().GetTxHash(tx);

  std::vector<CardanoTransaction::TxWitness> witnesses;
  for (const auto& input : tx.inputs()) {
    if (!address_map->contains(input.utxo_address)) {
      return false;
    }
    auto& key_id = address_map->at(input.utxo_address);

    auto signature_pair =
        keyring_service().SignMessageByCardanoKeyring(account_id, key_id, hash);
    if (!signature_pair) {
      return false;
    }
    static_assert(kCardanoWitnessSize ==
                  kEd25519PublicKeySize + kEd25519SignatureSize);
    std::array<uint8_t, kCardanoWitnessSize> witness_bytes = {};
    base::span(witness_bytes)
        .first<kEd25519PublicKeySize>()
        .copy_from_nonoverlapping(signature_pair->pubkey);
    base::span(witness_bytes)
        .last<kEd25519SignatureSize>()
        .copy_from_nonoverlapping(signature_pair->signature);
    witnesses.emplace_back(witness_bytes);
  }

  tx.SetWitnesses(std::move(witnesses));

  return tx.IsSigned();
}

void CardanoWalletService::OnPostTransaction(
    CardanoTransaction cardano_transaction,
    SignAndPostTransactionCallback callback,
    base::expected<std::string, std::string> txid) {
  if (!txid.has_value()) {
    std::move(callback).Run("", std::move(cardano_transaction), txid.error());
    return;
  }

  std::move(callback).Run(txid.value(), std::move(cardano_transaction), "");
}

void CardanoWalletService::GetTransactionStatus(
    const std::string& chain_id,
    const std::string& txid,
    GetTransactionStatusCallback callback) {
  CHECK(IsCardanoNetwork(chain_id));
  GetCardanoRpc(chain_id)->GetTransaction(
      txid, base::BindOnce(&CardanoWalletService::OnGetTransactionStatus,
                           weak_ptr_factory_.GetWeakPtr(), txid,
                           std::move(callback)));
}

void CardanoWalletService::OnGetTransactionStatus(
    const std::string& txid,
    GetTransactionStatusCallback callback,
    base::expected<std::optional<cardano_rpc::Transaction>, std::string>
        transaction) {
  if (!transaction.has_value()) {
    std::move(callback).Run(base::unexpected(transaction.error()));
    return;
  }

  if (!transaction.value().has_value()) {
    std::move(callback).Run(base::ok(false));
    return;
  }

  if (base::HexEncodeLower(transaction.value()->tx_hash) != txid) {
    std::move(callback).Run(base::unexpected(WalletInternalErrorMessage()));
    return;
  }

  std::move(callback).Run(base::ok(true));
}

std::vector<mojom::CardanoAddressPtr> CardanoWalletService::GetUsedAddresses(
    const mojom::AccountIdPtr& account_id) {
  CHECK(IsCardanoAccount(account_id));

  // We always have one address for a cardano account.
  auto address = keyring_service().GetCardanoAddress(
      account_id,
      mojom::CardanoKeyId::New(mojom::CardanoKeyRole::kExternal, 0));
  if (!address) {
    return {};
  }

  std::vector<mojom::CardanoAddressPtr> result;
  result.push_back(std::move(address));
  return result;
}

std::vector<mojom::CardanoAddressPtr> CardanoWalletService::GetUnusedAddresses(
    const mojom::AccountIdPtr& account_id) {
  CHECK(IsCardanoAccount(account_id));

  // We always have one address for a cardano account. So don't have unused
  // addresses.
  return {};
}

mojom::CardanoAddressPtr CardanoWalletService::GetChangeAddress(
    const mojom::AccountIdPtr& account_id) {
  CHECK(IsCardanoAccount(account_id));

  // We always have one address for a cardano account which is a change address
  // also.
  return keyring_service().GetCardanoAddress(
      account_id,
      mojom::CardanoKeyId::New(mojom::CardanoKeyRole::kExternal, 0));
}

cardano_rpc::CardanoRpc* CardanoWalletService::GetCardanoRpc(
    const std::string& chain_id) {
  if (chain_id == mojom::kCardanoMainnet) {
    return &cardano_mainnet_rpc_;
  }
  if (chain_id == mojom::kCardanoTestnet) {
    return &cardano_testnet_rpc_;
  }
  NOTREACHED() << chain_id;
}

void CardanoWalletService::SetUrlLoaderFactoryForTesting(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  cardano_mainnet_rpc_.SetUrlLoaderFactoryForTesting(  // IN-TEST
      url_loader_factory);
  cardano_testnet_rpc_.SetUrlLoaderFactoryForTesting(  // IN-TEST
      url_loader_factory);
}

}  // namespace brave_wallet
