/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_wallet_service.h"

#include <stdint.h>

#include <algorithm>
#include <map>
#include <optional>
#include <utility>
#include <vector>

#include "base/notreached.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_create_transaction_task.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_get_utxos_task.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_serializer.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/cardano_address.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_wallet {

namespace {

mojom::CardanoBalancePtr BalanceFromUtxos(GetCardanoUtxosTask::UtxoMap& utxos) {
  auto result = mojom::CardanoBalance::New();

  for (auto& items : utxos) {
    for (auto& utxo : items.second) {
      result->total_balance += utxo.lovelace_amount;
    }
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
      cardano_rpc_(network_manager, url_loader_factory) {}

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
    base::expected<GetCardanoUtxosTask::UtxoMap, std::string> utxos) {
  if (!utxos.has_value()) {
    std::move(callback).Run(nullptr, utxos.error());
    return;
  }
  std::move(callback).Run(BalanceFromUtxos(utxos.value()), std::nullopt);
}

void CardanoWalletService::DiscoverNextUnusedAddress(
    const mojom::AccountIdPtr& account_id,
    mojom::CardanoKeyRole role,
    DiscoverNextUnusedAddressCallback callback) {
  CHECK(IsCardanoAccount(account_id));

  // TODO(apaymyshev): this always returns first address.
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
  for (auto& address : *addresses) {
    if (auto cardano_address =
            CardanoAddress::FromString(address->address_string)) {
      cardano_addresses.push_back(std::move(*cardano_address));
    }
  }

  auto& task = get_cardano_utxo_tasks_.emplace_back(
      std::make_unique<GetCardanoUtxosTask>(
          *this, GetNetworkForCardanoAccount(account_id),
          std::move(cardano_addresses)),
      std::move(callback));

  task.first->set_callback(
      base::BindOnce(&CardanoWalletService::OnGetUtxosTaskDone,
                     weak_ptr_factory_.GetWeakPtr(), task.first.get()));

  task.first->ScheduleWorkOnTask();
}

void CardanoWalletService::OnGetUtxosTaskDone(
    GetCardanoUtxosTask* task,
    base::expected<GetCardanoUtxosTask::UtxoMap, std::string> result) {
  auto it = std::ranges::find(get_cardano_utxo_tasks_, task,
                              [](auto& t) { return t.first.get(); });
  if (it == get_cardano_utxo_tasks_.end()) {
    NOTREACHED();
  }

  auto cb = std::move(it->second);
  get_cardano_utxo_tasks_.erase(it);

  std::move(cb).Run(std::move(result));
}

void CardanoWalletService::CreateTransaction(
    mojom::AccountIdPtr account_id,
    const CardanoAddress& address_to,
    uint64_t amount,
    bool sending_max_amount,
    CreateTransactionCallback callback) {
  CHECK(IsCardanoAccount(account_id));

  auto& task = create_transaction_tasks_.emplace_back(
      std::make_unique<CreateCardanoTransactionTask>(
          *this, account_id, address_to, amount, sending_max_amount),
      std::move(callback));

  task.first->set_callback(
      base::BindOnce(&CardanoWalletService::OnCreateTransactionTaskDone,
                     weak_ptr_factory_.GetWeakPtr(), task.first.get()));

  task.first->ScheduleWorkOnTask();
}

void CardanoWalletService::OnCreateTransactionTaskDone(
    CreateCardanoTransactionTask* task,
    base::expected<CardanoTransaction, std::string> result) {
  auto it = std::ranges::find(create_transaction_tasks_, task,
                              [](auto& t) { return t.first.get(); });
  if (it == create_transaction_tasks_.end()) {
    NOTREACHED();
  }

  auto cb = std::move(it->second);
  create_transaction_tasks_.erase(it);

  std::move(cb).Run(std::move(result));
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
      CardanoSerializer::SerializeTransaction(cardano_transaction);

  cardano_rpc_.PostTransaction(
      GetNetworkForCardanoAccount(account_id), serialized_transaction,
      base::BindOnce(&CardanoWalletService::OnPostTransaction,
                     weak_ptr_factory_.GetWeakPtr(),
                     std::move(cardano_transaction), std::move(callback)));
}

bool CardanoWalletService::SignTransactionInternal(
    CardanoTransaction& tx,
    const mojom::AccountIdPtr& account_id) {
  auto addresses = keyring_service().GetCardanoAddresses(account_id);
  if (!addresses || addresses->empty()) {
    return false;
  }

  std::map<CardanoAddress, mojom::CardanoKeyIdPtr> address_map;
  for (auto& addr : *addresses) {
    auto cardano_address = CardanoAddress::FromString(addr->address_string);
    if (!cardano_address) {
      continue;
    }
    address_map.emplace(std::move(*cardano_address),
                        std::move(addr->payment_key_id));
  }

  auto hash = CardanoSerializer::GetTxHash(tx);

  std::vector<CardanoTransaction::TxWitness> witnesses;
  for (auto& input : tx.inputs()) {
    if (!address_map.contains(input.utxo_address)) {
      return false;
    }
    auto& key_id = address_map.at(input.utxo_address);

    auto signature =
        keyring_service().SignMessageByCardanoKeyring(account_id, key_id, hash);
    if (!signature) {
      return false;
    }

    witnesses.emplace_back(*signature);
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

void CardanoWalletService::SetUrlLoaderFactoryForTesting(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  cardano_rpc_.SetUrlLoaderFactoryForTesting(  // IN-TEST
      std::move(url_loader_factory));
}

}  // namespace brave_wallet
