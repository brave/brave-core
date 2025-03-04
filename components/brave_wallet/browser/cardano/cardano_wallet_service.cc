/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_wallet_service.h"

#include <stdint.h>

#include <algorithm>
#include <optional>
#include <utility>

#include "base/notreached.h"
#include "base/strings/string_number_conversions.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_get_utxos_task.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_utils.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_wallet {

namespace {

mojom::CardanoBalancePtr BalanceFromUtxos(GetCardanoUtxosTask::UtxoMap& utxos) {
  auto result = mojom::CardanoBalance::New();

  for (auto& items : utxos) {
    for (auto& utxo : items.second) {
      if (auto lovelace_amount = GetLovelaceAmountFromUtxo(utxo)) {
        result->total_balance += *lovelace_amount;
      }
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

// void CardanoWalletService::OnGetCardanoUtxosTaskDone(
//     GetCardanoUtxosTask* task,
//     base::expected<GetCardanoUtxosTask::UtxoMap, std::string> result) {
//   auto it = std::ranges::find(get_cardano_utxo_tasks_, task,
//                               [](auto& t) { return t.first.get(); });
//   if (it == get_cardano_utxo_tasks_.end()) {
//     NOTREACHED();
//   }

//   auto cb = std::move(it->second);
//   get_cardano_utxo_tasks_.erase(it);

//   if (!result.has_value()) {
//     std::move(cb).Run(nullptr, result.error());
//     return;
//   }

//   std::move(cb).Run(BalanceFromUtxos(result.value()), std::nullopt);
// }

void CardanoWalletService::GetUtxos(mojom::AccountIdPtr account_id,
                                    GetUtxosCallback callback) {
  auto addresses = keyring_service().GetCardanoAddresses(account_id);
  if (!addresses) {
    std::move(callback).Run(base::unexpected(WalletInternalErrorMessage()));
    return;
  }

  auto& task = get_cardano_utxo_tasks_.emplace_back(
      std::make_unique<GetCardanoUtxosTask>(
          *this, GetNetworkForCardanoAccount(account_id),
          std::move(*addresses)),
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

  // if (!result.has_value()) {
  //   std::move(cb).Run(nullptr, result.error());
  //   return;
  // }

  std::move(cb).Run(std::move(result));
}

void CardanoWalletService::SetUrlLoaderFactoryForTesting(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  cardano_rpc_.SetUrlLoaderFactoryForTesting(  // IN-TEST
      std::move(url_loader_factory));
}

}  // namespace brave_wallet
