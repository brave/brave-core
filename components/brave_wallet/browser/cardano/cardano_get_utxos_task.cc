/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_get_utxos_task.h"

#include <stdint.h>

#include <optional>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/memory/scoped_refptr.h"
#include "base/task/bind_post_task.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_wallet_service.h"
#include "brave/components/brave_wallet/common/common_utils.h"

namespace brave_wallet {

GetCardanoUtxosTask::GetCardanoUtxosTask(
    CardanoWalletService& cardano_wallet_service,
    const std::string& chain_id,
    std::vector<CardanoAddress> addresses)
    : cardano_wallet_service_(cardano_wallet_service),
      chain_id_(chain_id),
      pending_addresses_(std::move(addresses)) {
  CHECK(IsCardanoNetwork(chain_id));
}

GetCardanoUtxosTask::~GetCardanoUtxosTask() = default;

void GetCardanoUtxosTask::Start(Callback callback) {
  callback_ = base::BindPostTaskToCurrentDefault(std::move(callback));
  FetchAllRequiredData();
}

cardano_rpc::CardanoRpc* GetCardanoUtxosTask::GetCardanoRpc() {
  return cardano_wallet_service_->GetCardanoRpc(chain_id_);
}

void GetCardanoUtxosTask::FetchAllRequiredData() {
  if (pending_addresses_.empty()) {
    StopWithResult(UtxoMap());
    return;
  }

  for (const auto& address : pending_addresses_) {
    GetCardanoRpc()->GetUtxoList(
        address.ToString(),
        base::BindOnce(&GetCardanoUtxosTask::OnGetUtxoList,
                       weak_ptr_factory_.GetWeakPtr(), address));
  }
}

void GetCardanoUtxosTask::OnGetUtxoList(
    CardanoAddress address,
    base::expected<cardano_rpc::UnspentOutputs, std::string> utxos) {
  if (!utxos.has_value()) {
    StopWithError(std::move(utxos.error()));
    return;
  }

  utxos_[address] = std::move(utxos.value());
  CHECK(std::erase(pending_addresses_, address));

  OnMaybeAllRequiredDataFetched();
}

bool GetCardanoUtxosTask::IsAllRequiredDataFetched() {
  return pending_addresses_.empty();
}

void GetCardanoUtxosTask::OnMaybeAllRequiredDataFetched() {
  if (IsAllRequiredDataFetched()) {
    StopWithResult(std::move(utxos_));
  }
}

void GetCardanoUtxosTask::StopWithError(std::string error_string) {
  weak_ptr_factory_.InvalidateWeakPtrs();

  std::move(callback_).Run(base::unexpected(std::move(error_string)));
}

void GetCardanoUtxosTask::StopWithResult(UtxoMap result) {
  weak_ptr_factory_.InvalidateWeakPtrs();

  std::move(callback_).Run(base::ok(std::move(result)));
}

}  // namespace brave_wallet
