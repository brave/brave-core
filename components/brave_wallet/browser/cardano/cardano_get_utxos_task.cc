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
#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_wallet_service.h"

namespace brave_wallet {

GetCardanoUtxosTask::GetCardanoUtxosTask(
    CardanoWalletService& cardano_wallet_service,
    const std::string& chain_id,
    std::vector<mojom::CardanoAddressPtr> addresses,
    GetCardanoUtxosTask::Callback callback)
    : cardano_wallet_service_(cardano_wallet_service),
      chain_id_(chain_id),
      addresses_(std::move(addresses)),
      callback_(std::move(callback)) {}

GetCardanoUtxosTask::~GetCardanoUtxosTask() = default;

void GetCardanoUtxosTask::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&GetCardanoUtxosTask::WorkOnTask,
                                weak_factory_.GetWeakPtr()));
}

void GetCardanoUtxosTask::MaybeSendRequests() {
  if (requests_sent_) {
    return;
  }
  requests_sent_ = true;

  if (addresses_.empty()) {
    result_ = UtxoMap();
    ScheduleWorkOnTask();
    return;
  }

  // Shuffle addresses so requests are always done in different order to
  // increase privacy a bit.
  base::RandomShuffle(addresses_.begin(), addresses_.end());

  for (const auto& address_info : addresses_) {
    cardano_wallet_service_->cardano_rpc().GetUtxoList(
        chain_id_, address_info->address_string,
        base::BindOnce(&GetCardanoUtxosTask::OnGetUtxoList,
                       weak_factory_.GetWeakPtr(), address_info->Clone()));
  }
}

void GetCardanoUtxosTask::OnGetUtxoList(
    mojom::CardanoAddressPtr address,
    base::expected<cardano_rpc::UnspentOutputs, std::string> utxos) {
  if (!utxos.has_value()) {
    error_ = utxos.error();
    return WorkOnTask();
  }

  utxos_[address->address_string] = std::move(utxos.value());

  CHECK(std::erase(addresses_, address));
  if (addresses_.empty()) {
    result_ = std::move(utxos_);
  }

  WorkOnTask();
}

void GetCardanoUtxosTask::WorkOnTask() {
  if (error_) {
    std::move(callback_).Run(this, base::unexpected(std::move(*error_)));
    return;
  }

  if (result_) {
    std::move(callback_).Run(this, base::ok(std::move(*result_)));
    return;
  }

  MaybeSendRequests();
}

}  // namespace brave_wallet
