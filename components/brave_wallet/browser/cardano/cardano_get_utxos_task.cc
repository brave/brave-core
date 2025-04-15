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

namespace brave_wallet {

GetCardanoUtxosTask::GetCardanoUtxosTask(
    CardanoWalletService& cardano_wallet_service,
    const std::string& chain_id,
    std::vector<CardanoAddress> addresses)
    : cardano_wallet_service_(cardano_wallet_service),
      chain_id_(chain_id),
      addresses_(std::move(addresses)) {}

GetCardanoUtxosTask::~GetCardanoUtxosTask() = default;

void GetCardanoUtxosTask::Start(Callback callback) {
  callback_ = base::BindPostTaskToCurrentDefault(std::move(callback));
  ScheduleWorkOnTask();
}

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

  for (const auto& address : addresses_) {
    cardano_wallet_service_->cardano_rpc().GetUtxoList(
        chain_id_, address.ToString(),
        base::BindOnce(&GetCardanoUtxosTask::OnGetUtxoList,
                       weak_factory_.GetWeakPtr(), address));
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

  CHECK(std::erase(addresses_, address));
  if (addresses_.empty()) {
    result_ = std::move(utxos_);
  }

  WorkOnTask();
}

void GetCardanoUtxosTask::WorkOnTask() {
  if (result_) {
    std::move(callback_).Run(base::ok(std::move(*result_)));
    return;
  }

  MaybeSendRequests();
}

void GetCardanoUtxosTask::StopWithError(std::string error_string) {
  std::move(callback_).Run(base::unexpected(std::move(error_string)));
}

}  // namespace brave_wallet
