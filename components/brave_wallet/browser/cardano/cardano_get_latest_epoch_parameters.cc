/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_get_latest_epoch_parameters.h"

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

GetLatestEpochParametersTask::GetLatestEpochParametersTask(
    CardanoWalletService& cardano_wallet_service,
    const std::string& chain_id,
    std::vector<mojom::CardanoAddressPtr> addresses)
    : cardano_wallet_service_(cardano_wallet_service),
      chain_id_(chain_id),
      addresses_(std::move(addresses)) {}

GetLatestEpochParametersTask::~GetLatestEpochParametersTask() = default;

void GetLatestEpochParametersTask::ScheduleWorkOnTask() {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&GetLatestEpochParametersTask::WorkOnTask,
                                weak_factory_.GetWeakPtr()));
}

void GetLatestEpochParametersTask::MaybeSendRequests() {
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
        base::BindOnce(&GetLatestEpochParametersTask::OnGetUtxoList,
                       weak_factory_.GetWeakPtr(), address_info->Clone()));
  }
}

void GetLatestEpochParametersTask::OnGetUtxoList(
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

void GetLatestEpochParametersTask::WorkOnTask() {
  if (error_) {
    std::move(callback_).Run(base::unexpected(std::move(*error_)));
    return;
  }

  if (result_) {
    std::move(callback_).Run(base::ok(std::move(*result_)));
    return;
  }

  MaybeSendRequests();
}

}  // namespace brave_wallet
